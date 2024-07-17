#pragma once

#include "../UnitTests.h"

#include "../../DataStructures/Intermediate/Data.h"
#include "../../DataStructures/RAPTOR/Data.h"
#include "../../DataStructures/CSA/Data.h"

#include "../../DataStructures/CSA/Data.h"

namespace UnitTests {

class ForwardCrossTransfers {

public:
    template<typename ALGORITHM>
    inline void check(ALGORITHM& algorithm, const std::string& algorithmName) {
        algorithm.run(StopId(0), StopId(4));
        RAPTOR::Profile result = algorithm.getProfile();
        UnitTests::check(result.size() == 2, "ForwardCrossTransfers (", algorithmName, "): Profile size should be 2 but is ", result.size());
        if (result.size() >= 1) {
            UnitTests::check(result[0].arrivalTime == 120, "ForwardCrossTransfers (", algorithmName, "): Arrival time should be 120 but is ", result[0].arrivalTime);
            UnitTests::check(result[0].departureTime == 100, "ForwardCrossTransfers (", algorithmName, "): Departure time should be 100 but is ", result[0].departureTime);
            UnitTests::check(result[0].numberOfTrips == 3, "ForwardCrossTransfers (", algorithmName, "): Number of trips should be 3 but is ", result[0].numberOfTrips);
        }
        if (result.size() >= 2) {
            UnitTests::check(result[1].arrivalTime == 125, "ForwardCrossTransfers (", algorithmName, "): Arrival time should be 125 but is ", result[1].arrivalTime);
            UnitTests::check(result[1].departureTime == 101, "ForwardCrossTransfers (", algorithmName, "): Departure time should be 101 but is ", result[1].departureTime);
            UnitTests::check(result[1].numberOfTrips == 2, "ForwardCrossTransfers (", algorithmName, "): Number of trips should be 2 but is ", result[1].numberOfTrips);
        }
    }

    template<typename ALGORITHM>
    inline void checkCSA(const std::string& algorithmName) {
        CSA::Data data = buildNetworkCSA();
        data.sortConnectionsAscendingByDepartureTime();
        ALGORITHM algorithm(data, data.transferGraph);
        check(algorithm, algorithmName);
    }

    template<typename ALGORITHM>
    inline void checkRAPTOR(const std::string& algorithmName) {
        RAPTOR::Data data = buildNetworkRAPTOR();
        RAPTOR::Data reverseData = data.reverseNetwork();
        ALGORITHM algorithm(data, reverseData);
        check(algorithm, algorithmName);
    }

protected:
    inline CSA::Data buildNetworkCSA() const noexcept {
        TransferEdgeList transferGraph;
        transferGraph.addVertices(6);

        transferGraph.set(Coordinates, Vertex(0), Geometry::Point(Construct::XY, 0.0, 0.5));
        transferGraph.set(Coordinates, Vertex(1), Geometry::Point(Construct::XY, 1.0, 0.8));
        transferGraph.set(Coordinates, Vertex(2), Geometry::Point(Construct::XY, 1.0, 0.2));
        transferGraph.set(Coordinates, Vertex(3), Geometry::Point(Construct::XY, 2.0, 0.8));
        transferGraph.set(Coordinates, Vertex(4), Geometry::Point(Construct::XY, 3.0, 0.5));
        transferGraph.set(Coordinates, Vertex(5), Geometry::Point(Construct::XY, 1.0, 0.5));

        transferGraph.addEdge(Vertex(1), Vertex(5)).set(TravelTime, 1);
        transferGraph.addEdge(Vertex(5), Vertex(1)).set(TravelTime, 1);
        transferGraph.addEdge(Vertex(5), Vertex(2)).set(TravelTime, 1);
        transferGraph.addEdge(Vertex(2), Vertex(5)).set(TravelTime, 1);

        std::vector<CSA::Stop> stops;
        stops.emplace_back("S", transferGraph.get(Coordinates, Vertex(0)),  5);
        stops.emplace_back("A", transferGraph.get(Coordinates, Vertex(1)),  5);
        stops.emplace_back("B", transferGraph.get(Coordinates, Vertex(2)),  5);
        stops.emplace_back("C", transferGraph.get(Coordinates, Vertex(3)),  5);
        stops.emplace_back("T", transferGraph.get(Coordinates, Vertex(4)),  5);

        std::vector<CSA::Trip> trips;
        trips.emplace_back("S -> A", "R1", 1);
        trips.emplace_back("S -> B", "R2", 1);
        trips.emplace_back("A -> C", "R3", 1);
        trips.emplace_back("C -> T", "R4", 1);
        trips.emplace_back("B -> T", "R5", 1);

        std::vector<CSA::Connection> connections;
        connections.emplace_back(StopId(0), StopId(1), 101, 105, TripId(0));
        connections.emplace_back(StopId(0), StopId(2), 100, 105, TripId(1));
        connections.emplace_back(StopId(1), StopId(3), 108, 110, TripId(2));
        connections.emplace_back(StopId(3), StopId(4), 118, 120, TripId(3));
        connections.emplace_back(StopId(2), StopId(4), 108, 125, TripId(4));

        return CSA::Data::FromInput(stops, connections, trips, transferGraph);
    }

    inline RAPTOR::Data buildNetworkRAPTOR() const noexcept {
        CSA::Data csa = buildNetworkCSA();
        Intermediate::Data inter = Intermediate::Data::FromCSA(csa);
        return RAPTOR::Data::FromIntermediate(inter);
    }

};

class BackwardCrossTransfers {

public:
    template<typename ALGORITHM>
    inline void check(ALGORITHM& algorithm, const std::string& algorithmName) {
        algorithm.run(StopId(4), StopId(0));
        RAPTOR::Profile result = algorithm.getProfile();
        UnitTests::check(result.size() == 2, "BackwardCrossTransfers (", algorithmName, "): Profile size should be 2 but is ", result.size());
        if (result.size() >= 1) {
            UnitTests::check(result[0].arrivalTime == 124, "BackwardCrossTransfers (", algorithmName, "): Arrival time should be 120 but is ", result[0].arrivalTime);
            UnitTests::check(result[0].departureTime == 100, "BackwardCrossTransfers (", algorithmName, "): Departure time should be 100 but is ", result[0].departureTime);
            UnitTests::check(result[0].numberOfTrips == 2, "BackwardCrossTransfers (", algorithmName, "): Number of trips should be 2 but is ", result[0].numberOfTrips);
        }
        if (result.size() >= 2) {
            UnitTests::check(result[1].arrivalTime == 125, "BackwardCrossTransfers (", algorithmName, "): Arrival time should be 125 but is ", result[1].arrivalTime);
            UnitTests::check(result[1].departureTime == 105, "BackwardCrossTransfers (", algorithmName, "): Departure time should be 105 but is ", result[1].departureTime);
            UnitTests::check(result[1].numberOfTrips == 3, "BackwardCrossTransfers (", algorithmName, "): Number of trips should be 3 but is ", result[1].numberOfTrips);
        }
    }

    template<typename ALGORITHM>
    inline void checkCSA(const std::string& algorithmName) {
        CSA::Data data = buildNetworkCSA();
        data.sortConnectionsAscendingByDepartureTime();
        ALGORITHM algorithm(data, data.transferGraph);
        check(algorithm, algorithmName);
    }

    template<typename ALGORITHM>
    inline void checkRAPTOR(const std::string& algorithmName) {
        RAPTOR::Data data = buildNetworkRAPTOR();
        RAPTOR::Data reverseData = data.reverseNetwork();
        ALGORITHM algorithm(data, reverseData);
        check(algorithm, algorithmName);
    }

protected:
    inline CSA::Data buildNetworkCSA() const noexcept {
        TransferEdgeList transferGraph;
        transferGraph.addVertices(6);

        transferGraph.set(Coordinates, Vertex(0), Geometry::Point(Construct::XY, 0.0, 0.5));
        transferGraph.set(Coordinates, Vertex(1), Geometry::Point(Construct::XY, 1.0, 0.8));
        transferGraph.set(Coordinates, Vertex(2), Geometry::Point(Construct::XY, 1.0, 0.2));
        transferGraph.set(Coordinates, Vertex(3), Geometry::Point(Construct::XY, 2.0, 0.8));
        transferGraph.set(Coordinates, Vertex(4), Geometry::Point(Construct::XY, 3.0, 0.5));
        transferGraph.set(Coordinates, Vertex(5), Geometry::Point(Construct::XY, 1.0, 0.5));

        transferGraph.addEdge(Vertex(1), Vertex(5)).set(TravelTime, 1);
        transferGraph.addEdge(Vertex(5), Vertex(1)).set(TravelTime, 1);
        transferGraph.addEdge(Vertex(5), Vertex(2)).set(TravelTime, 1);
        transferGraph.addEdge(Vertex(2), Vertex(5)).set(TravelTime, 1);

        std::vector<CSA::Stop> stops;
        stops.emplace_back("S", transferGraph.get(Coordinates, Vertex(0)),  5);
        stops.emplace_back("A", transferGraph.get(Coordinates, Vertex(1)),  5);
        stops.emplace_back("B", transferGraph.get(Coordinates, Vertex(2)),  5);
        stops.emplace_back("C", transferGraph.get(Coordinates, Vertex(3)),  5);
        stops.emplace_back("T", transferGraph.get(Coordinates, Vertex(4)),  5);

        std::vector<CSA::Trip> trips;
        trips.emplace_back("S <- A", "R1", 1);
        trips.emplace_back("S <- B", "R2", 1);
        trips.emplace_back("A <- C", "R3", 1);
        trips.emplace_back("C <- T", "R4", 1);
        trips.emplace_back("B <- T", "R5", 1);

        std::vector<CSA::Connection> connections;
        connections.emplace_back(StopId(1), StopId(0), 120, 124, TripId(0));
        connections.emplace_back(StopId(2), StopId(0), 120, 125, TripId(1));
        connections.emplace_back(StopId(3), StopId(1), 115, 117, TripId(2));
        connections.emplace_back(StopId(4), StopId(3), 105, 107, TripId(3));
        connections.emplace_back(StopId(4), StopId(2), 100, 117, TripId(4));

        return CSA::Data::FromInput(stops, connections, trips, transferGraph);
    }

    inline RAPTOR::Data buildNetworkRAPTOR() const noexcept {
        CSA::Data csa = buildNetworkCSA();
        Intermediate::Data inter = Intermediate::Data::FromCSA(csa);
        return RAPTOR::Data::FromIntermediate(inter);
    }

};

class CrossTransfers {

public:
    template<typename ALGORITHM>
    inline void checkCSA(const std::string& algorithmName) {
        ForwardCrossTransfers().checkCSA<ALGORITHM>(algorithmName);
        BackwardCrossTransfers().checkCSA<ALGORITHM>(algorithmName);
    }

    template<typename ALGORITHM>
    inline void checkRAPTOR(const std::string& algorithmName) {
        ForwardCrossTransfers().checkRAPTOR<ALGORITHM>(algorithmName);
        BackwardCrossTransfers().checkRAPTOR<ALGORITHM>(algorithmName);
    }

};

}
