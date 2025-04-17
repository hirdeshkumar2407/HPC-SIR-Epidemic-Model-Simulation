#include "GridSimulation.h"
#include <mpi.h>
#include <unordered_map>

GridSimulation::GridSimulation(const SIRModel& m, int mpiRank, int mpiSize) 
    : model(m), rank(mpiRank), size(mpiSize) {}

void GridSimulation::setGrid(const std::vector<SIRCell>& initialGrid) {
    grid = initialGrid;
}

std::vector<SIRCell>& GridSimulation::getGrid() {
    return grid;
}

int GridSimulation::getLocalSize() const {
    return grid.size();
}

void GridSimulation::setNeighborMap(const std::unordered_map<int, std::vector<int>>& map) {
    neighborMap = map;
}

void GridSimulation::updateGrid() {
    std::vector<SIRCell> newGrid = grid;
    for (size_t i = 0; i < grid.size(); ++i) {
        newGrid[i] = model.rk4Step(grid[i]);
    }
    grid = newGrid;
}

void GridSimulation::updateGridNew() {
    std::vector<SIRCell> newGrid = grid;

    for (size_t i = 0; i < grid.size(); ++i) {
        std::vector<SIRCell> neighbors;

        // Get neighbors from map (if exists)
        if (neighborMap.find(i) != neighborMap.end()) {
            const std::vector<int>& neighborIDs = neighborMap[i];
            for (int j : neighborIDs) {
                if (j >= 0 && j < static_cast<int>(grid.size())) {
                    neighbors.push_back(grid[j]);
                }
            }
        }

        // Use model to compute update using neighbors
        newGrid[i] = model.rk4StepWithNeighbors(grid[i], neighbors);
    }

    grid = newGrid;
}


std::vector<std::vector<double>> GridSimulation::runSimulation() {
    std::vector<std::vector<double>> results; // [time, avg_S, avg_I, avg_R]
    
    for (int step = 0; step < model.getNumSteps(); ++step) {
        // Update grid
        //updateGrid();
        updateGridNew();
        
        // Compute average S, I, R
        double sumS = 0, sumI = 0, sumR = 0;
        for (auto &cell : grid) {
            sumS += cell.getS();
            sumI += cell.getI();
            sumR += cell.getR();
        }
        
        double avgS = sumS / grid.size();
        double avgI = sumI / grid.size();
        double avgR = sumR / grid.size();
        double timeVal = step * model.getDt();
        
        results.push_back({timeVal, avgS, avgI, avgR});
        
        // Synchronize processes
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    return results;
}