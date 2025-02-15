//
// Created by Yantuan Xian on 2024/6/28.
//

#ifndef UTILITY_H
#define UTILITY_H
#include <fstream>
#include "CoDeSEG.h"

/**
 * Load graph edges form text file, each edge per line with format, 'src_node tgt_node weight'
 * @param filename the path to text file contains edges
 * @param edges array of edges read from text file
 * @param weighted indicate if edges are weighted
 * @return edge number, or -1: can not open the text file
 */
long load_edges(const std::string &filename, EdgeArray &edges, bool weighted = false);

/**
 * Save communities to text file, each community per line
 * @param filename the path to community text file
 * @param cmty a map contains communities detected by algrithm.
 * @param filter_nodes a filter set to select nodes
 * @return community number, or -1: can not open the text file
 */
long save_cmty(const std::string &filename, const CmtyMap &cmty, const std::set<std::string> &filter_nodes);

/**
 * Load graph nodes from ground truth community file
 * @param filename The ground truth community file
 * @param nodes The set to store loaded notes
 * @return Count of loaded communities
 */
std::size_t load_ground_truth_nodes(const std::string &filename, std::set<std::string> &nodes);

/**
 * Show communities in std::cout
 * @param communities The map of communities
 */
void show_communities(const std::unordered_map<unsigned long, std::set<std::string> > &communities);

#endif //UTILITY_H
