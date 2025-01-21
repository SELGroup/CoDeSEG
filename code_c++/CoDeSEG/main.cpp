#include <iostream>
#include <string>
#include <regex>

#include "lib/CxxOpts.h"
#include "lib/CoDeSEG.h"
#include "lib/DynamicArray.h"
#include "lib/Utility.h"

void load_demo(EdgeArray &edges) {
    const auto demo_edges = std::vector<Edge>{
        {"a", "b", 0.8}, {"a", "e", 0.5},
        {"b", "c", 0.1}, {"b", "e", 0.9},
        {"c", "d", 0.3}, {"c", "f", 0.5},
        {"d", "e", 0.2}, {"d", "f", 0.3}
    };

    for (const auto &e: demo_edges) {
        edges.push(e);
    }
}

int main(const int argc, const char **argv) {
    cxxopts::Options options(
        "codeseg",
        "CoDeSEG: A fast Community Detection for large-scale networks via Structural Entropy Game");

    options.add_options()
            ("i,input", "Input file of graph edge list", cxxopts::value<std::string>())
            ("o,output", "Output file of communities, each line a community", cxxopts::value<std::string>())
            ("t,ground_truth", "Ground truth file of communities, each line a community", cxxopts::value<std::string>())
            ("w,weighted", "Indicate edge is weighted or not", cxxopts::value<bool>()->default_value("false"))
            ("x,overlapping", "Detect overlapping communities", cxxopts::value<bool>()->default_value("false"))
            ("v,verbose", "Print detection iteration messages", cxxopts::value<bool>()->default_value("false"))
            ("m,minimum", "Minimum number of nodes per community.", cxxopts::value<unsigned int>()->default_value("5"))
            ("e,entropy_threshold", "Max iterations", cxxopts::value<float>()->default_value("0.25"))
            ("n,max_iteration", "Max iterations", cxxopts::value<int>()->default_value("5"))
            ("p,parallel", "Max iterations", cxxopts::value<int>()->default_value("1"))
            ("h,help", "Print usage");

    const auto args = options.parse(argc, argv);

    // print the usage help.
    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // load graph edges from a demo graph or an edge text file.
    EdgeArray edges;
    if (args.count("i") <= 0) {
        load_demo(edges);
    } else {
        const auto edge_file = args["i"].as<std::string>();
        const auto weighted = args["w"].as<bool>();

        auto tm_start = timing_start();
        const auto edge_num = load_edges(edge_file, edges, weighted);
        auto tm_duration = timing_stop(tm_start);

        if (edge_num >= 0) {
            std::cout << edge_num << " edges loaded from: " << edge_file
                    << ", in " << tm_duration << "ms" << std::endl;
        } else {
            std::cout << "Can not open the edge text file: " << edge_file << std::endl;
            exit(0);
        }
    }

    const auto max_iter = args["n"].as<int>();
    const auto ovlp = args["x"].as<bool>();
    const auto verb = args["v"].as<bool>();
    const auto num_workers = args["p"].as<int>();
    const auto se_tau = args["e"].as<float>();

    CoDeSEG codeseg;

    auto tm_start = timing_start();
    codeseg.add_edges(edges);
    auto tm_duration = timing_stop(tm_start);

    std::cout << "graph built in " << tm_duration << "ms." << std::endl;

    tm_start = timing_start();
    if (num_workers <= 1) {
        codeseg.detect_cmty(max_iter, ovlp, verb, se_tau);
    } else {
        codeseg.detect_cmty(num_workers, max_iter, ovlp, verb, se_tau);
    }
    tm_duration = timing_stop(tm_start);
    std::cout << "detection done in " << tm_duration << "ms." << std::endl;

    const auto cmty = codeseg.communities();
    // std::cout << cmty.size() << " communities detected in graph, #nodes: "
    //         << codeseg.get_nodes().size() << ", #edges: " << edges.size() << std::endl;

    std::set<std::string> gt_nodes;
    if (args.count("t") > 0) {
        const auto truth_cmty = args["t"].as<std::string>();
        const auto num_cmty = load_ground_truth_nodes(truth_cmty, gt_nodes);
        std::cout << gt_nodes.size() << " unique nodes in " << num_cmty << " communities, "
                << truth_cmty << std::endl;
    }

    if (args.count("o") <= 0) {
        for (const auto &[key, nodes]: cmty) {
            for (const auto &node: nodes) {
                std::cout << node << " ";
            }
            std::cout << std::endl;
        }
    } else {
        const auto cmty_file = args["o"].as<std::string>();
        tm_start = timing_start();
        const auto cmty_num = save_cmty(cmty_file, cmty, gt_nodes);
        tm_duration = timing_stop(tm_start);

        // std::cout << "write " << cmty_num << " communities to file: " << cmty_file <<
        //         ", in " << tm_duration << "ms." << std::endl;
    }

    return 0;
}
