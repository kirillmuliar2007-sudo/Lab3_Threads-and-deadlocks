#include <windows.h>
#include <iostream>
#include <vector>
#include <ctime>

using namespace std;

struct MarkerData {
    int id;
    bool active;
    HANDLE hEventStuck;
    HANDLE hEventResume;
    HANDLE hEventTerminate;
};

int* arr = nullptr;
int arr_size = 0;
CRITICAL_SECTION cs;
HANDLE hStartAll;

void PrintArray() {
    EnterCriticalSection(&cs);
    cout << " Массив:  ";
    for (int i = 0; i < arr_size; i++) {
        cout << arr[i] << " ";
    }
    cout << "\n";
    LeaveCriticalSection(&cs);
}

DWORD WINAPI MarkerThread(LPVOID param) {
    MarkerData* data = static_cast<MarkerData*>(param);
    WaitForSingleObject(hStartAll, INFINITE);
    srand(data->id);
    int marked_count = 0;

    while (true) {
        int idx = rand() % arr_size;

        EnterCriticalSection(&cs);
        if (arr[idx] == 0) {
            LeaveCriticalSection(&cs);
            Sleep(5);

            EnterCriticalSection(&cs);
            arr[idx] = data->id;
            LeaveCriticalSection(&cs);

            marked_count++;
            Sleep(5);
        }
        else {
            LeaveCriticalSection(&cs);

            EnterCriticalSection(&cs);
            cout << "\nНомер потока " << data->id << " количество: " << marked_count << " индекс: " << idx << "\n";
            LeaveCriticalSection(&cs);

            SetEvent(data->hEventStuck);

            HANDLE wait_events[2] = { data->hEventResume, data->hEventTerminate };
            DWORD wait_res = WaitForMultipleObjects(2, wait_events, FALSE, INFINITE);

            if (wait_res == WAIT_OBJECT_0 + 1) {
                EnterCriticalSection(&cs);
                for (int i = 0; i < arr_size; i++) {
                    if (arr[i] == data->id) {
                        arr[i] = 0;
                    }
                }
                LeaveCriticalSection(&cs);
                return 0;
            }
        }
    }
    return 0;
}

int main() {
    setlocale(LC_ALL, "ru");
    InitializeCriticalSection(&cs);

    cout << "Размерность: ";
    cin >> arr_size;

    arr = new int[arr_size];
    for (int i = 0; i < arr_size; i++) arr[i] = 0;

    int num_markers = 0;
    cout << "Количество потоков: ";
    cin >> num_markers;

    hStartAll = CreateEvent(NULL, TRUE, FALSE, NULL);
    vector<MarkerData> markers(num_markers);
    vector<HANDLE> hThreads(num_markers);

    for (int i = 0; i < num_markers; i++) {
        markers[i].id = i + 1;
        markers[i].active = true;
        markers[i].hEventStuck = CreateEvent(NULL, FALSE, FALSE, NULL);
        markers[i].hEventResume = CreateEvent(NULL, FALSE, FALSE, NULL);
        markers[i].hEventTerminate = CreateEvent(NULL, FALSE, FALSE, NULL);

        hThreads[i] = CreateThread(NULL, 0, MarkerThread, &markers[i], 0, NULL);
    }

    SetEvent(hStartAll);

    while (true) {
        int active_count = 0;
        for (int i = 0; i < num_markers; i++) {
            if (markers[i].active) active_count++;
        }
        if (active_count == 0) break;

        for (int i = 0; i < num_markers; i++) {
            if (markers[i].active) {
                WaitForSingleObject(markers[i].hEventStuck, INFINITE);
            }
        }

        PrintArray();

        int kill_id = 0;
        while (true) {
            cout << "Номер потока, чью работу нужно завершить: ";
            cin >> kill_id;
            if (kill_id >= 1 && kill_id <= num_markers && markers[kill_id - 1].active) {
                break;
            }
        }

        SetEvent(markers[kill_id - 1].hEventTerminate);
        WaitForSingleObject(hThreads[kill_id - 1], INFINITE);
        markers[kill_id - 1].active = false;
        CloseHandle(hThreads[kill_id - 1]);

        PrintArray();

        for (int i = 0; i < num_markers; i++) {
            if (markers[i].active) {
                SetEvent(markers[i].hEventResume);
            }
        }
    }

    CloseHandle(hStartAll);
    for (int i = 0; i < num_markers; i++) {
        CloseHandle(markers[i].hEventStuck);
        CloseHandle(markers[i].hEventResume);
        CloseHandle(markers[i].hEventTerminate);
    }
    delete[] arr;
    DeleteCriticalSection(&cs);
    return 0;
}
