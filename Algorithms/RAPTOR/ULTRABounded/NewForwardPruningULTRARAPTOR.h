#pragma once

#include <vector>

#include "../../../DataStructures/Container/Map.h"
#include "../../../DataStructures/Container/Set.h"
#include "../../../DataStructures/RAPTOR/Data.h"
#include "../../../DataStructures/RAPTOR/Entities/ArrivalLabel.h"

#include "../InitialTransfers.h"
#include "../Profiler.h"

namespace RAPTOR {

template<typename PROFILER = NoProfiler, typename INITIAL_TRANSFERS = BucketCHInitialTransfers>
class NewForwardPruningULTRARAPTOR {

public:
    using Profiler = PROFILER;
    using InitialTransferType = INITIAL_TRANSFERS;
    using Type = NewForwardPruningULTRARAPTOR<Profiler, InitialTransferType>;

    struct RoundLabel {
        RoundLabel(const int arrivalTime, const int walkingDistance = INFTY) :
            arrivalTime(arrivalTime),
            walkingDistance(walkingDistance) {
        }

        int arrivalTime;
        int walkingDistance;
    };

private:
    using Round = std::vector<RoundLabel>;

public:
    NewForwardPruningULTRARAPTOR(const Data& data, InitialTransferType& initialTransfers, Profiler& profiler) :
        data(data),
        initialTransfers(initialTransfers),
        stopsUpdatedByRoute(data.numberOfStops()),
        stopsUpdatedByTransfer(data.numberOfStops()),
        routesServingUpdatedStops(data.numberOfRoutes()),
        sourceVertex(noVertex),
        targetVertex(noVertex),
        sourceStop(noStop),
        targetStop(noStop),
        sourceDepartureTime(never),
        arrivalSlack(INFTY),
        profiler(profiler) {
        Assert(data.hasImplicitBufferTimes(), "Departure buffer times have to be implicit!");
    }

    inline void run(const Vertex source, const int departureTime, const Vertex target, const double arrivalFactor, const double) noexcept {
        profiler.startPhase();
        clear();
        initialize(source, departureTime, target, arrivalFactor);
        profiler.donePhase(PHASE_INITIALIZATION);
        profiler.startPhase();
        relaxInitialTransfers();
        profiler.donePhase(PHASE_TRANSFERS);

        for (size_t i = 0; !stopsUpdatedByTransfer.empty(); i++) {
            profiler.startPhase();
            startNewRound();
            profiler.donePhase(PHASE_INITIALIZATION);
            profiler.startPhase();
            collectRoutesServingUpdatedStops();
            profiler.donePhase(PHASE_COLLECT);
            profiler.startPhase();
            scanRoutes();
            profiler.donePhase(PHASE_SCAN);
            profiler.startPhase();
            startNewRound();
            profiler.donePhase(PHASE_INITIALIZATION);
            profiler.startPhase();
            relaxIntermediateTransfers();
            profiler.donePhase(PHASE_TRANSFERS);
        }
        profiler.startPhase();
        computeAnchorLabels();
        profiler.donePhase(PHASE_INITIALIZATION);
    }

    inline const std::vector<WalkingParetoLabel>& getAnchorLabels() const noexcept {
        return anchorLabels;
    }

    inline int getArrivalTime(const StopId stop, const size_t round) const noexcept {
        return rounds[std::min(2 * round + 1, rounds.size() - 1)][stop].arrivalTime;
    }

private:
    inline void clear() noexcept {
        stopsUpdatedByRoute.clear();
        stopsUpdatedByTransfer.clear();
        routesServingUpdatedStops.clear();
        anchorLabels.clear();
        rounds.clear();
    }

    inline void initialize(const Vertex source, const int departureTime, const Vertex target, const double factor) noexcept {
        sourceVertex = source;
        targetVertex = target;
        sourceStop = data.isStop(source) ? StopId(source) : StopId(data.numberOfStops() + 1);
        targetStop = data.isStop(target) ? StopId(target) : StopId(data.numberOfStops());
        sourceDepartureTime = departureTime;
        arrivalSlack = 1 + factor;
        startNewRound();
        arrivalByRoute(sourceStop, sourceDepartureTime, 0);
        startNewRound();
    }

    inline void collectRoutesServingUpdatedStops() noexcept {
        for (const StopId stop : stopsUpdatedByTransfer) {
            Assert(data.isStop(stop), "Stop " << stop << " is out of range!");
            const int arrivalTime = previousRound()[stop].arrivalTime;
            Assert(arrivalTime < never, "Updated stop has arrival time = never!");
            for (const RouteSegment& route : data.routesContainingStop(stop)) {
                Assert(data.isRoute(route.routeId), "Route " << route.routeId << " is out of range!");
                Assert(data.stopIds[data.firstStopIdOfRoute[route.routeId] + route.stopIndex] == stop, "RAPTOR data contains invalid route segments!");
                if (route.stopIndex + 1 == data.numberOfStopsInRoute(route.routeId)) continue;
                if (data.lastTripOfRoute(route.routeId)[route.stopIndex].departureTime < arrivalTime) continue;
                if (routesServingUpdatedStops.contains(route.routeId)) {
                    routesServingUpdatedStops[route.routeId] = std::min(routesServingUpdatedStops[route.routeId], route.stopIndex);
                } else {
                    routesServingUpdatedStops.insert(route.routeId, route.stopIndex);
                }
            }
        }
    }

    inline void scanRoutes() noexcept {
        stopsUpdatedByRoute.clear();
        for (const RouteId route : routesServingUpdatedStops.getKeys()) {
            profiler.countMetric(METRIC_ROUTES);
            StopIndex stopIndex = routesServingUpdatedStops[route];
            const size_t tripSize = data.numberOfStopsInRoute(route);
            Assert(stopIndex < tripSize - 1, "Cannot scan a route starting at/after the last stop (Route: " << route << ", StopIndex: " << stopIndex << ", TripSize: " << tripSize << ")!");

            const StopId* stops = data.stopArrayOfRoute(route);
            const StopEvent* trip = data.lastTripOfRoute(route);
            StopId stop = stops[stopIndex];
            Assert(trip[stopIndex].departureTime >= previousRound()[stop].arrivalTime, "Cannot scan a route after the last trip has departed (Route: " << route << ", Stop: " << stop << ", StopIndex: " << stopIndex << ", Time: " << previousRound()[stop].arrivalTime << ", LastDeparture: " << trip[stopIndex].departureTime << ")!");
            int walkingDistance = previousRound()[stop].walkingDistance;

            StopIndex parentIndex = stopIndex;
            const StopEvent* firstTrip = data.firstTripOfRoute(route);
            while (stopIndex < tripSize - 1) {
                while ((trip > firstTrip) && ((trip - tripSize + stopIndex)->departureTime >= previousRound()[stop].arrivalTime)) {
                    trip -= tripSize;
                    parentIndex = stopIndex;
                    walkingDistance = previousRound()[stop].walkingDistance;
                }
                stopIndex++;
                stop = stops[stopIndex];
                profiler.countMetric(METRIC_ROUTE_SEGMENTS);
                arrivalByRoute(stop, trip[stopIndex].arrivalTime, walkingDistance);
            }
        }
    }

    inline void relaxInitialTransfers() noexcept {
        initialTransfers.template run(sourceVertex, targetVertex, arrivalSlack);
        for (const Vertex stop : initialTransfers.getForwardPOIs()) {
            if (stop == targetStop) continue;
            Assert(data.isStop(stop), "Reached POI " << stop << " is not a stop!");
            const int walkingDistance = initialTransfers.getForwardDistance(stop);
            Assert(walkingDistance != INFTY, "Vertex " << stop << " was not reached!");
            const int arrivalTime = sourceDepartureTime + walkingDistance;
            arrivalByTransfer(StopId(stop), arrivalTime, walkingDistance);
        }
        if (initialTransfers.getDistance() != INFTY) {
            const int arrivalTime = sourceDepartureTime + initialTransfers.getDistance();
            arrivalByTransfer(targetStop, arrivalTime, initialTransfers.getDistance());
        }
        if (data.isStop(sourceStop)) stopsUpdatedByTransfer.insert(sourceStop);
    }

    inline void relaxIntermediateTransfers() noexcept {
        routesServingUpdatedStops.clear();
        const std::vector<StopId> stopsToScan = stopsUpdatedByTransfer.getValues();
        stopsUpdatedByTransfer.clear();
        for (const StopId stop : stopsUpdatedByRoute) {
            const RoundLabel& parentLabel = previousRound()[stop];
            relaxShortcuts(stop, parentLabel);
            const int backwardDistance = initialTransfers.getBackwardDistance(stop);
            if (backwardDistance != INFTY) {
                const int arrivalTime = parentLabel.arrivalTime + backwardDistance;
                const int walkingDistance = parentLabel.walkingDistance + backwardDistance;
                arrivalByTransfer(targetStop, arrivalTime, walkingDistance);
            }
            stopsUpdatedByTransfer.insert(stop);
        }
        for (const StopId stop : stopsToScan) {
            relaxShortcuts(stop, previousRound()[stop]);
        }
    }

    inline void relaxShortcuts(const StopId stop, const RoundLabel& parentLabel) noexcept {
        for (const Edge edge : data.transferGraph.edgesFrom(stop)) {
            profiler.countMetric(METRIC_EDGES);
            Assert(data.isStop(data.transferGraph.get(ToVertex, edge)), "Graph contains edges to non stop vertices!");
            const StopId toStop = StopId(data.transferGraph.get(ToVertex, edge));
            const int edgeWeight = data.transferGraph.get(TravelTime, edge);
            const int arrivalTime = parentLabel.arrivalTime + edgeWeight;
            const int walkingDistance = parentLabel.walkingDistance + edgeWeight;
            arrivalByTransfer(toStop, arrivalTime, walkingDistance);
        }
    }

    inline Round& currentRound() noexcept {
        Assert(!rounds.empty(), "Cannot return current round, because no round exists!");
        return rounds.back();
    }

    inline Round& previousRound() noexcept {
        Assert(rounds.size() >= 2, "Cannot return previous round, because less than two rounds exist!");
        return rounds[rounds.size() - 2];
    }

    inline void startNewRound() noexcept {
        if (rounds.empty()) {
            rounds.emplace_back(data.numberOfStops() + 2, never);
        } else {
            rounds.emplace_back(rounds.back());
        }
    }

    inline void arrival(const StopId stop, const int time, const int walkingDistance, IndexedSet<false, StopId>& updatedStops, Metric metric) noexcept {
        if ((currentRound()[targetStop].arrivalTime - sourceDepartureTime) * arrivalSlack < time - sourceDepartureTime) return;
        if (currentRound()[stop].arrivalTime <= time) return;
        profiler.countMetric(metric);
        currentRound()[stop].arrivalTime = time;
        currentRound()[stop].walkingDistance = walkingDistance;
        if (data.isStop(stop)) updatedStops.insert(stop);
    }

    inline void arrivalByRoute(const StopId stop, const int time, const int walkingDistance) noexcept {
        arrival(stop, time, walkingDistance, stopsUpdatedByRoute, METRIC_STOPS_BY_TRIP);
    }

    inline void arrivalByTransfer(const StopId stop, const int time, const int walkingDistance) noexcept {
        arrival(stop, time, walkingDistance, stopsUpdatedByTransfer, METRIC_STOPS_BY_TRANSFER);
    }

    inline void computeAnchorLabels() noexcept {
        for (size_t i = 1; i < rounds.size(); i += 2) {
            if (rounds[i][targetStop].arrivalTime >= (anchorLabels.empty() ? never : anchorLabels.back().arrivalTime)) continue;
            anchorLabels.emplace_back(rounds[i][targetStop].arrivalTime, rounds[i][targetStop].walkingDistance, i / 2);
        }
        Vector::reverse(anchorLabels);
    }

private:
    const Data& data;
    InitialTransferType& initialTransfers;

    std::vector<Round> rounds;

    IndexedSet<false, StopId> stopsUpdatedByRoute;
    IndexedSet<false, StopId> stopsUpdatedByTransfer;
    IndexedMap<StopIndex, false, RouteId> routesServingUpdatedStops;

    Vertex sourceVertex;
    Vertex targetVertex;
    StopId sourceStop;
    StopId targetStop;
    int sourceDepartureTime;
    double arrivalSlack;

    std::vector<WalkingParetoLabel> anchorLabels;

    Profiler& profiler;

};

}