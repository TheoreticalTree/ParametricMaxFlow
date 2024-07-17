#pragma once

#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include <string>

#include "../../Shell/Shell.h"

#include "../../Helpers/String/String.h"
#include "../../Helpers/String/Enumeration.h"
#include "../../Helpers/String/TextFileUtils.h"
#include "../../Helpers/Vector/Vector.h"
#include "../../Helpers/Vector/Permutation.h"

#include "../../Helpers/Assert.h"
#include "../../Helpers/Debug.h"
#include "../../Helpers/Timer.h"
#include "../../Helpers/Calendar.h"
#include "../../Helpers/MultiThreading.h"
#include "../../Helpers/IO/File.h"
#include "../../Helpers/IO/CSVData.h"
#include "../../Helpers/IO/ParserCSV.h"
#include "../../Helpers/Vector/Vector.h"

#include "../../DataStructures/Graph/Graph.h"
#include "../../DataStructures/RAPTOR/Data.h"
#include "../../DataStructures/Partition/VertexPartition.h"
#include "../../DataStructures/Partition/NestedDissection.h"
#include "../../DataStructures/Partition/SampleGraph.h"

#include "../../Algorithms/GreedyVertexColoring.h"
#include "../../Algorithms/Partition/GreedyCenters.h"
#include "../../Algorithms/Partition/InertialFlow.h"
#include "../../Algorithms/MaxFlowMinCut/Dinic.h"

#include "../../Visualization/Color.h"
#include "../../Visualization/PDF.h"
#include "../../Visualization/PNG.h"
#include "../../Visualization/SVG.h"
#include "../../Visualization/TimeTableVisualization.h"
#include "../../Visualization/CsaVisualization.h"

using namespace Shell;

class DrawGreedyCells : public ParameterizedCommand {

public:
    DrawGreedyCells(BasicShell& shell) :
        ParameterizedCommand(shell, "drawGreedyCells", "Computes a greedy partition for a RAPTOR network and visualizes the result.") {
        addParameter("RAPTOR binary");
        addParameter("Graphics output file");
        addParameter("Format", { "pdf", "png", "svg" });
        addParameter("Number of cells");
        addParameter("Initial cell coverage");
        addParameter("Final cell coverage");
        addParameter("Border coverage");
    }

    virtual void execute() noexcept {
        const std::string format = getParameter("Format");
        if (format == "pdf") {
            draw<PDF>();
        } else if (format == "png") {
            draw<PNG>();
        } else {
            draw<SVG>();
        }
    }

private:
    template<typename FORMAT>
    inline void draw() const noexcept {
        using Visualization = TimeTableVisualization<FORMAT>;
        RAPTOR::Data data = RAPTOR::Data::FromBinary(getParameter("RAPTOR binary"));
        data.printInfo();
        const size_t numberOfCells = getParameter<size_t>("Number of cells");
        const double initialCellCoverage = getParameter<double>("Initial cell coverage");
        const double finalCellCoverage = getParameter<double>("Final cell coverage");
        const double borderCoverage = getParameter<double>("Border coverage");
        std::cout << "Computing Greedy Centers." << std::endl;
        GreedyCenters<true> gc(data);
        gc.run(numberOfCells, initialCellCoverage);
        std::cout << "Computing Partition." << std::endl;
        VertexPartition vp(gc.getVertexPartition(numberOfCells, finalCellCoverage, borderCoverage));
        std::cout << "   Number of cells = " << vp.numberOfCells() << std::endl;
        std::cout << "Drawing Partition." << std::endl;
        Visualization doc = Visualization::FromRAPTOR(getParameter("Graphics output file"), data, 0.3);
        doc.drawPartition(vp);
        doc.newPage();
        doc.close();
        std::cout << "Drawing Network." << std::endl;
        doc.drawRoutesByType();
    }
};

class ComputeNetworkNestedDissection : public ParameterizedCommand {

public:
    ComputeNetworkNestedDissection(BasicShell& shell) :
        ParameterizedCommand(shell, "computeNetworkNestedDissection", "Computes a nested dissection for a RAPTOR network and visualizes the result.") {
        addParameter("RAPTOR binary");
        addParameter("Graphics output file");
        addParameter("Format", { "pdf", "png", "svg" });
        addParameter("Number of cells");
        addParameter("Fixed vertices");
        addParameter("Transitive routes?", "false");
        addParameter("Number of threads", "-1");
        addParameter("Pin multiplier", "1");
    }

    virtual void execute() noexcept {
        const std::string format = getParameter("Format");
        if (format == "pdf") {
            draw<PDF>();
        } else if (format == "png") {
            draw<PNG>();
        } else {
            draw<SVG>();
        }
    }

private:
    template<typename FORMAT>
    inline void draw() const noexcept {
        using Visualization = TimeTableVisualization<FORMAT>;
        RAPTOR::Data data = RAPTOR::Data::FromBinary(getParameter("RAPTOR binary"));
        data.printInfo();
        NestedDissection nd = computeNestedDissection(data);
        std::cout << "Drawing Partition." << std::endl;
        const std::string outputFile = getParameter("Graphics output file");
        Visualization doc = Visualization::FromRAPTOR(outputFile, data, 0.3);
        doc.drawPartition(nd, true);
        doc.close();
        std::cout << "Saving Partition." << std::endl;
        nd.serialize(outputFile + ".nd");
    }

    inline NestedDissection computeNestedDissection(const RAPTOR::Data& data) const noexcept {
        std::cout << "Computing Transfer Graph." << std::endl;
        RAPTOR::TransferGraph transferGraph = getParameter<bool>("Transitive routes?") ? data.minTravelTimeTransitiveGraph() : data.minTravelTimeGraph();
        std::cout << "Initializing Inertial Flow." << std::endl;
        InertialFlowOnVertices<true> inertialFlow(transferGraph, data.getCoordinates());
        const int numberOfThreads = getNumberOfThreads();
        const size_t numberOfCells = getParameter<size_t>("Number of cells");
        const double fixedVertices = getParameter<double>("Fixed vertices");
        if (numberOfThreads <= 0) {
            std::cout << "Computing Partition (sequential)." << std::endl;
            return inertialFlow.run(transferGraph, numberOfCells, fixedVertices);
        } else {
            const size_t pinMultiplier = getParameter<size_t>("Pin multiplier");
            const ThreadPinning threadPinning(numberOfThreads, pinMultiplier);
            std::cout << "Computing Partition (parallel with " << numberOfThreads << " threads)." << std::endl;
            return inertialFlow.run(threadPinning, transferGraph, numberOfCells, fixedVertices);
        }
    }

    inline int getNumberOfThreads() const noexcept {
        if (getParameter("Number of threads") == "max") {
            return numberOfCores();
        }
        else {
            return getParameter<int>("Number of threads");
        }
    }
};

class RunGraphInertialFlow : public ParameterizedCommand {

public:
    RunGraphInertialFlow(BasicShell& shell) :
        ParameterizedCommand(shell, "runGraphInertialFlow", "Computes a vertex partition for a RAPTOR network and visualizes the result.") {
        addParameter("Input graph");
        addParameter("Output file");
        addParameter("Format", { "pdf", "png", "svg" });
        addParameter("Maximum cell size");
        addParameter("Fixed vertices");
        addParameter("Number of threads", "-1");
        addParameter("Pin multiplier", "1");
    }

    virtual void execute() noexcept {
        const std::string format = getParameter("Format");
        if (format == "pdf") {
            draw<PDF>();
        } else if (format == "png") {
            draw<PNG>();
        } else {
            draw<SVG>();
        }
    }

private:
    using InertialFlowType = InertialFlowOnEdges<true, InertialFlow::QUANTITY>;

    template<typename FORMAT>
    inline void draw() const noexcept {
        using Visualization = MapVisualization<FORMAT>;
        TransferGraph graph(getParameter("Input graph"));
        Graph::printInfo(graph);
        graph.printAnalysis();
        std::cout << "Initializing InertialFlow." << std::endl;
        InertialFlowType inertialFlow(graph, graph[Coordinates]);
        VertexPartition vp = computeVertexPartition(inertialFlow);
        validatePartition(vp, graph);
        std::cout << "Drawing partition." << std::endl;
        std::vector<int> cellColors = greedyVertexColors(vp.getCellGraph(graph));
        const std::string outputFile = getParameter("Output file");
        Visualization doc = Visualization(outputFile, graph[Coordinates], 0.3);
        std::vector<Vertex> boundaryVertices = vp.getBorderVertices(graph);
        for (const Vertex vertex : boundaryVertices) {
            const size_t cell = vp.getCellIdOfVertex(vertex);
            doc.drawPoint(graph.get(Coordinates, vertex), cyclicColor(cellColors[cell]), Icon::Dot, 10);
        }
        for (const VertexPartition::CutEdge edge : vp.getCutEdges(graph)) {
            doc.drawLine(graph.get(Coordinates, edge.from), graph.get(Coordinates, edge.to));
        }
        doc.close();
        std::cout << "Saving partition." << std::endl;
        vp.serialize(outputFile + ".vp");
    }

    inline VertexPartition computeVertexPartition(InertialFlowType& inertialFlow) const noexcept {
        const int numberOfThreads = getNumberOfThreads();
        const size_t maxCellSize = getParameter<size_t>("Maximum cell size");
        const double fixedVertices = getParameter<double>("Fixed vertices");
        if (numberOfThreads <= 0) {
            std::cout << "Computing partition (sequential)." << std::endl;
            return inertialFlow.runOnConnectedComponents(maxCellSize, fixedVertices);
        } else {
            const size_t pinMultiplier = getParameter<size_t>("Pin multiplier");
            const ThreadPinning threadPinning(numberOfThreads, pinMultiplier);
            std::cout << "Computing partition (parallel with " << numberOfThreads << " threads)." << std::endl;
            return inertialFlow.runOnConnectedComponents(threadPinning, maxCellSize, fixedVertices);
        }
    }

    inline void validatePartition(const VertexPartition& vp, const TransferGraph& graph) const noexcept {
        std::cout << "Validating partition." << std::endl;
        StronglyConnectedComponents<TransferGraph> scc(graph);
        scc.run();
        for (size_t cell = 0; cell < vp.numberOfCells(); cell++) {
            if (vp.getCell(cell).empty()) continue;
            int component = scc.getComponent(vp.getCell(cell)[0]);
            for (const Vertex vertex : vp.getCell(cell)) {
                if (scc.getComponent(vertex) != component) {
                    std::cout << "Vertices of cell " << cell << " not in the same component!" << std::endl;
                    continue;
                }
            }
        }
    }

    inline int getNumberOfThreads() const noexcept {
        if (getParameter("Number of threads") == "max") {
            return numberOfCores();
        }
        else {
            return getParameter<int>("Number of threads");
        }
    }
};

class RunNetworkInertialFlow : public ParameterizedCommand {

public:
    RunNetworkInertialFlow(BasicShell& shell) :
        ParameterizedCommand(shell, "runNetworkInertialFlow", "Computes a vertex partition for a RAPTOR network and visualizes the result.\ngraph type:\n       0         -  minTravelTimeGraph\n       1         -  minTravelTimeTransitiveGraph\n       filename  -  Graph::DynamicFlow file\n") {
        addParameter("RAPTOR binary");
        addParameter("Graphics output file");
        addParameter("Format", { "pdf", "png", "svg" });
        addParameter("Number of cells");
        addParameter("Fixed vertices");
        addParameter("Flow graph");
        addParameter("Transitive routes?", "false");
        addParameter("Stops only?", "true");
        addParameter("Number of threads", "-1");
        addParameter("Pin multiplier", "1");
    }

    virtual void execute() noexcept {
        const std::string format = getParameter("Format");
        if (format == "pdf") {
            draw<PDF>();
        } else if (format == "png") {
            draw<PNG>();
        } else {
            draw<SVG>();
        }
    }

private:
    template<typename FORMAT>
    inline void draw() const noexcept {
        using Visualization = TimeTableVisualization<FORMAT>;
        RAPTOR::Data data = RAPTOR::Data::FromBinary(getParameter("RAPTOR binary"));
        data.printInfo();
        const std::string flowGraph = getParameter("Flow graph");
        VertexPartition vp;
        if (flowGraph.size() == 1) {
            std::cout << "Computing Transfer Graph." << std::endl;
            RAPTOR::TransferGraph transferGraph = getParameter<bool>("Transitive routes?") ? data.minTravelTimeTransitiveGraph() : data.minTravelTimeGraph();
            Graph::printInfo(transferGraph);
            transferGraph.printAnalysis();
            std::cout << "Initializing Inertial Flow." << std::endl;
            InertialFlowOnEdges<true> inertialFlow(transferGraph, data.getCoordinates());
            vp = computeVertexPartition(inertialFlow);
        } else {
            std::cout << "Loading Flow Graph." << std::endl;
            DynamicFlowGraph graph;
            graph.readBinary(flowGraph);
            Graph::printInfo(graph);
            graph.printAnalysis();
            std::cout << "Initializing Inertial Flow." << std::endl;
            InertialFlowOnEdges<true> inertialFlow(graph, data.getCoordinates(), graph[Capacity]);
            vp = computeVertexPartition(inertialFlow);
        }
        std::cout << "Drawing Partition." << std::endl;
        std::vector<int> cellColors = greedyVertexColors(vp.getCellGraph(data.transferGraph));
        const bool stopsOnly = getParameter<bool>("Stops only?");
        const std::string outputFile = getParameter("Graphics output file");
        Visualization doc = Visualization::FromRAPTOR(outputFile, data, 0.3);
        doc.drawPartition(vp, cellColors, stopsOnly);
        doc.newPage();
        doc.drawPartition(vp, cellColors, stopsOnly);
        doc.drawCutEdges(vp.getCutEdges(data.transferGraph));
        doc.close();
        std::cout << "Saving Partition." << std::endl;
        vp.serialize(outputFile + ".vp");
    }


    inline VertexPartition computeVertexPartition(InertialFlowOnEdges<true>& inertialFlow) const noexcept {
        const int numberOfThreads = getNumberOfThreads();
        const size_t numberOfCells = getParameter<size_t>("Number of cells");
        const double fixedVertices = getParameter<double>("Fixed vertices");
        if (numberOfThreads <= 0) {
            std::cout << "Computing Partition (sequential)." << std::endl;
            return inertialFlow.run(numberOfCells, fixedVertices);
        } else {
            const size_t pinMultiplier = getParameter<size_t>("Pin multiplier");
            const ThreadPinning threadPinning(numberOfThreads, pinMultiplier);
            std::cout << "Computing Partition (parallel with " << numberOfThreads << " threads)." << std::endl;
            return inertialFlow.run(threadPinning, numberOfCells, fixedVertices);
        }
    }

    inline int getNumberOfThreads() const noexcept {
        if (getParameter("Number of threads") == "max") {
            return numberOfCores();
        }
        else {
            return getParameter<int>("Number of threads");
        }
    }
};

class DrawVertexPartition : public ParameterizedCommand {

public:
    DrawVertexPartition(BasicShell& shell) :
        ParameterizedCommand(shell, "drawVertexPartition", "Visualizes a vertex partition.") {
        addParameter("RAPTOR binary");
        addParameter("Partition file");
        addParameter("Graphics output file");
        addParameter("Format", { "pdf", "png", "svg" });
        addParameter("Stops only?", "true");
    }

    virtual void execute() noexcept {
        const std::string format = getParameter("Format");
        if (format == "pdf") {
            draw<PDF>();
        } else if (format == "png") {
            draw<PNG>();
        } else {
            draw<SVG>();
        }
    }

private:
    template<typename FORMAT>
    inline void draw() const noexcept {
        using Visualization = TimeTableVisualization<FORMAT>;
        RAPTOR::Data data = RAPTOR::Data::FromBinary(getParameter("RAPTOR binary"));
        data.printInfo();
        const std::string outputFile = getParameter("Graphics output file");
        VertexPartition vp(outputFile);
        std::cout << "Drawing Partition." << std::endl;
        std::vector<int> cellColors = greedyVertexColors(vp.getCellGraph(data.transferGraph));
        Visualization doc = Visualization::FromRAPTOR(outputFile, data, 0.3);
        const bool stopsOnly = getParameter<bool>("Stops only?");
        doc.drawPartition(vp, cellColors, stopsOnly);
        doc.newPage();
        doc.drawPartition(vp, cellColors, stopsOnly);
        doc.drawCutEdges(vp.getCutEdges(data.transferGraph));
        doc.close();
    }
};

class DrawNestedDissection : public ParameterizedCommand {

public:
    DrawNestedDissection(BasicShell& shell) :
        ParameterizedCommand(shell, "drawNestedDissection", "Visualizes a nested dissection.") {
        addParameter("RAPTOR binary");
        addParameter("Partition file");
        addParameter("Graphics output file");
        addParameter("Format", { "pdf", "png", "svg" });
    }

    virtual void execute() noexcept {
        const std::string format = getParameter("Format");
        if (format == "pdf") {
            draw<PDF>();
        } else if (format == "png") {
            draw<PNG>();
        } else {
            draw<SVG>();
        }
    }

private:
    template<typename FORMAT>
    inline void draw() const noexcept {
        using Visualization = TimeTableVisualization<FORMAT>;
        RAPTOR::Data data = RAPTOR::Data::FromBinary(getParameter("RAPTOR binary"));
        data.printInfo();
        const std::string outputFile = getParameter("Graphics output file");
        NestedDissection nd(outputFile);
        std::cout << "Drawing Partition." << std::endl;
        Visualization doc = Visualization::FromRAPTOR(outputFile, data, 0.3);
        doc.drawPartition(nd, true);
        doc.close();
    }
};

class ComputeSampleGraph : public ParameterizedCommand {

public:
    ComputeSampleGraph(BasicShell& shell) :
        ParameterizedCommand(shell, "computeSampleGraph", "Computes a sampled parent graph.") {
        addParameter("RAPTOR binary");
        addParameter("Nested dissection file");
        addParameter("Separator level");
        addParameter("Number of samples");
        addParameter("Weighted?");
        addParameter("Use min transfer times?");
        addParameter("Output file");
        addParameter("Number of threads", "-1");
        addParameter("Pin multiplier", "1");
    }

    virtual void execute() noexcept {
        RAPTOR::Data data = RAPTOR::Data::FromBinary(getParameter("RAPTOR binary"));
        data.printInfo();
        NestedDissection nd(getParameter("Nested dissection file"));
        const size_t level = getParameter<size_t>("Separator level");
        const size_t numberOfSamples = getParameter<size_t>("Number of samples");
        const int numberOfThreads = getNumberOfThreads();
        const size_t pinMultiplier = getParameter<size_t>("Pin multiplier");
        const ThreadPinning threadPinning(numberOfThreads, pinMultiplier);
        const DynamicFlowGraph graph = Graph::generateSampleGraph(threadPinning, data, nd, level, numberOfSamples, getParameter<bool>("Weighted?"), getParameter<bool>("Use min transfer times?"));
        Graph::printInfo(graph);
        graph.printAnalysis();
        graph.writeBinary(getParameter("Output file"));
    }

private:
    inline int getNumberOfThreads() const noexcept {
        if (getParameter("Number of threads") == "max") {
            return numberOfCores();
        }
        else {
            return getParameter<int>("Number of threads");
        }
    }
};