#pragma once
#include <windows.h>
#include <iostream>
#include <vector>

struct SharedContext {
    int* arr = nullptr;
    int arrSize = 0;
    int markerCount = 0;

    CRITICAL_SECTION cs;
    HANDLE hStartEvent = NULL;

    std::vector<HANDLE> hThreads;
    std::vector<HANDLE> hCannotContinueEvents;
    std::vector<HANDLE> hContinueEvents;
    std::vector<HANDLE> hTerminateEvents;
    std::vector<bool> isAlive;

    SharedContext(int size, int count) : arrSize(size), markerCount(count) {
        arr = new int[arrSize]();
        InitializeCriticalSection(&cs);
        hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        hThreads.resize(markerCount, NULL);
        hCannotContinueEvents.resize(markerCount, NULL);
        hContinueEvents.resize(markerCount, NULL);
        hTerminateEvents.resize(markerCount, NULL);
        isAlive.resize(markerCount, true);

        for (int i = 0; i < markerCount; ++i) {
            hCannotContinueEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
            hContinueEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            hTerminateEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        }
    }

    ~SharedContext() {
        for (int i = 0; i < markerCount; ++i) {
            if (hThreads[i]) CloseHandle(hThreads[i]);
            if (hCannotContinueEvents[i]) CloseHandle(hCannotContinueEvents[i]);
            if (hContinueEvents[i]) CloseHandle(hContinueEvents[i]);
            if (hTerminateEvents[i]) CloseHandle(hTerminateEvents[i]);
        }
        if (hStartEvent) CloseHandle(hStartEvent);
        DeleteCriticalSection(&cs);
        delete[] arr;
    }

    void ClearElementsById(int id) {
        EnterCriticalSection(&cs);
        for (int i = 0; i < arrSize; ++i) {
            if (arr[i] == id) {
                arr[i] = 0;
            }
        }
        LeaveCriticalSection(&cs);
    }

    void PrintArray() {
        EnterCriticalSection(&cs);
        std::cout << "Массив: ";
        for (int i = 0; i < arrSize; ++i) {
            std::cout << arr[i] << " ";
        }
        std::cout << "\n";
        LeaveCriticalSection(&cs);
    }
};

struct MarkerArgs {
    SharedContext* ctx;
    int id;
};

inline DWORD WINAPI MarkerThread(LPVOID lpParam) {
    MarkerArgs* args = static_cast<MarkerArgs*>(lpParam);
    SharedContext* ctx = args->ctx;
    int id = args->id;
    int idx = id - 1;
    delete args;

    WaitForSingleObject(ctx->hStartEvent, INFINITE);
    srand(id);

    int markedCount = 0;

    while (true) {
        int r = rand();
        int targetIndex = r % ctx->arrSize;

        EnterCriticalSection(&ctx->cs);
        if (ctx->arr[targetIndex] == 0) {
            Sleep(5);
            ctx->arr[targetIndex] = id;
            markedCount++;
            Sleep(5);
            LeaveCriticalSection(&ctx->cs);
            continue;
        }
        else {
            std::cout << "Поток marker #" << id
                << " | Помечено элементов: " << markedCount
                << " | Невозможно пометить индекс: " << targetIndex << "\n";
            LeaveCriticalSection(&ctx->cs);

            SetEvent(ctx->hCannotContinueEvents[idx]);

            HANDLE waitEvents[2] = { ctx->hContinueEvents[idx], ctx->hTerminateEvents[idx] };
            DWORD waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0 + 1) {
                ctx->ClearElementsById(id);
                break;
            }
        }
    }

    return 0;
}