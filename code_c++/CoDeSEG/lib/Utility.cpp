//
// Created by Yantuan Xian on 2024/6/28.
//
#include "Utility.h"
#include "CoDeSEG.h"
#include <fstream>
#include <iostream>
#include <sstream>

void show_communities(const std::unordered_map<unsigned long, std::set<std::string> > &communities) {
    std::cout << "{ ";
    for (const auto &[key, values]: communities) {
        std::cout << key << ": [ ";
        for (const auto &v: values) {
            std::cout << v << ", ";
        }
        std::cout << "], ";
    }
    std::cout << " }" << std::endl;
}


long load_edges(const std::string &filename, EdgeArray &edges, bool weighted) {
    long count = -1;
    if (std::ifstream file(filename); file.is_open()) {
        std::string line;
        count = 0;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream line_stream(line);
            Node src_node, tgt_node;
            float weight = 1.0;
            if (!weighted) {
                line_stream >> src_node >> tgt_node;
            } else {
                line_stream >> src_node >> tgt_node >> weight;
            }
            edges.push(Edge(src_node, tgt_node, weight));
            count++;
        }

        file.close();
    }
    return count;
}


long save_cmty(const std::string &filename, const CmtyMap &cmty, const std::set<std::string> &filter_nodes) {
    long count = 0;
    if (std::ofstream cmty_file(filename); cmty_file.is_open()) {
        for (const auto &[key, nodes]: cmty) {
            long cmty_count = 0;
            for (const auto &node: nodes) {
                if (filter_nodes.empty()) {
                    cmty_file << node << " ";
                    cmty_count++;
                } else {
                    if (filter_nodes.find(node) != filter_nodes.end()) {
                        cmty_file << node << " ";
                        cmty_count++;
                    }
                }
            }
            if (cmty_count > 0) {
                cmty_file << std::endl;
                count++;
            }
        }

        cmty_file.close();
    }
    return count;
}


std::size_t load_ground_truth_nodes(const std::string &filename, std::set<std::string> &nodes) {
    std::size_t num_cmty = 0;
    if (std::ifstream file(filename); file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::size_t num_cmty_node = 0;
            std::istringstream line_stream(line);
            std::string node;
            while (line_stream) {
                line_stream >> node;
                // std::cout << node << " ";
                nodes.insert(node);
                num_cmty_node++;
            }

            if (num_cmty_node > 0) num_cmty++;
        }
        file.close();
    }
    return num_cmty;
}
