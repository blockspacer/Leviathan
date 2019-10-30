// Leviathan Game Engine
// Copyright (c) 2012-2019 Henri Hyyryläinen
#pragma once
#include "Define.h"
// ------------------------------------ //
#include "../ForwardDeclarations.h"
#include <cstddef>
#include <vector>

namespace Leviathan {

//! \brief Mainly a FPS limiter
//! \todo Merging this with scope times TimingMonitor would make a nice performance tracking
//! tool
class RenderingStatistics {
public:
    DLLEXPORT RenderingStatistics();
    DLLEXPORT ~RenderingStatistics();

    DLLEXPORT void RenderingStart();
    DLLEXPORT void RenderingEnd();

    DLLEXPORT bool CanRenderNow(int maxfps, int& TimeSinceLastFrame);

    DLLEXPORT void ReportStats(DataStore* dstore);

private:
    void HalfMinuteMark();
    void SecondMark();

    static void MakeSureHasEnoughRoom(std::vector<int>& tarvec, const size_t& accessspot);

    // ------------------------------------ //
    int64_t HalfMinuteStartTime;
    int64_t SecondStartTime;
    int64_t RenderingStartTime;
    int64_t RenderingEndTime;
    // measured values //
    int Frames;

    int FPS;
    int RenderMCRSeconds;

    int MinFPS;
    int MaxFPS;
    int AverageFps;

    int MaxFrameTime;
    int MinFrameTime;
    int AverageRenderTime;

    int DoubtfulCancel;

    bool IsFirstFrame;

    // stored values //
    std::vector<int> LastMinuteFPS;
    size_t LastMinuteFPSPos;

    std::vector<int> LastMinuteRenderTimes;
    size_t LastMinuteRenderTimesPos;


    bool EraseOld;
};

} // namespace Leviathan
