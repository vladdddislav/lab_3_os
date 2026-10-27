#include "marker_lab.h"
#include <iostream>
#include <vector>

int main() {
    setlocale(LC_ALL, "Russian");

    int arrSize = 0;
    std::cout << "Введите размерность массива: ";
    std::cin >> arrSize;

    int markerCount = 0;
    std::cout << "Введите количество потоков marker: ";
    std::cin >> markerCount;

    SharedContext ctx(arrSize, markerCount);

    for (int i = 0; i < markerCount; ++i) {
        MarkerArgs* args = new MarkerArgs{ &ctx, i + 1 };
        ctx.hThreads[i] = CreateThread(NULL, 0, MarkerThread, args, 0, NULL);
    }

    SetEvent(ctx.hStartEvent);

    int activeMarkers = markerCount;

    while (activeMarkers > 0) {
        std::vector<HANDLE> activeWaitEvents;
        for (int i = 0; i < markerCount; ++i) {
            if (ctx.isAlive[i]) {
                activeWaitEvents.push_back(ctx.hCannotContinueEvents[i]);
            }
        }

        WaitForMultipleObjects(
            static_cast<DWORD>(activeWaitEvents.size()),
            activeWaitEvents.data(),
            TRUE,
            INFINITE
        );

        ctx.PrintArray();

        int targetId = 0;
        while (true) {
            std::cout << "Введите порядковый номер потока marker для завершения (1 - " << markerCount << "): ";
            std::cin >> targetId;
            if (targetId >= 1 && targetId <= markerCount && ctx.isAlive[targetId - 1]) {
                break;
            }
            std::cout << "Некорректный номер или поток уже завершен. Повторите ввод.\n";
        }

        int targetIdx = targetId - 1;

        SetEvent(ctx.hTerminateEvents[targetIdx]);
        WaitForSingleObject(ctx.hThreads[targetIdx], INFINITE);
        ctx.isAlive[targetIdx] = false;
        activeMarkers--;

        ctx.PrintArray();

        for (int i = 0; i < markerCount; ++i) {
            if (ctx.isAlive[i]) {
                ResetEvent(ctx.hCannotContinueEvents[i]);
                SetEvent(ctx.hContinueEvents[i]);
            }
        }
    }

    std::cout << "Все потоки завершили работу.\n";
    return 0;
}