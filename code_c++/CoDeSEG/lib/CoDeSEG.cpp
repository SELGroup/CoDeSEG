
#include "CoDeSEG.h"
#include <cmath>
#include <iostream>

#include "ThreadPool.h"

using namespace std::chrono;

CoDeSEG &CoDeSEG::add_edges(EdgeArray &edges) {
    for (unsigned long i = 0; i < edges.size(); i++) {
        const auto &[src, tgt, weight] = edges[i];
        const auto &[src_pair, is_new_src] = node_idx.emplace(src, node_idx.size());
        if (is_new_src) {
            nodes.push(src);
            graph_adj.push(NodeAdj());
        }

        const auto [tgt_pair, is_new_tgt] = node_idx.emplace(tgt, node_idx.size());
        if (is_new_tgt) {
            nodes.push(tgt);
            graph_adj.push(NodeAdj());
        }

        const auto src_idx = src_pair->second;
        const auto tgt_idx = tgt_pair->second;

        graph_adj[src_idx][tgt_idx] = weight;
        graph_adj[tgt_idx][src_idx] = weight;
    }
    return *this;
}

void CoDeSEG::_init_cluster() {
    graph_vol = 0.;
    graph_se_max = 0.;

    for (size_t i = 0; i < nodes.size(); i++) {
        NodeAdj node_adj = graph_adj[i];
        FloatValue deg = 0.;
        for (const auto [neib, weight]: node_adj) {
            deg += weight;
        }
        graph_vol += deg;

        node_deg.push(deg);
        node_cmty.push(i);

        cmty_size.push(1);
        cmty_se_sum.push(0.);
        cmty_version.push(0);
    }

    cmty_vol.copy_from(node_deg);
    cmty_cut.copy_from(node_deg);

    for (size_t i = 0; i < nodes.size(); i++) {
        const auto p = node_deg[i] / graph_vol;
        graph_se_max += -p * log(p);
    }
}

float CoDeSEG::_delta_leave(const NodeIdx i, const NodeIdx cmty_idx, const FloatValue deg_i_cmty,
                            const bool i_in_cmty) {
    if (i_in_cmty && std::abs(cmty_vol[cmty_idx] - node_deg[i]) < 1e-8) return 0.;

    const auto i_deg = node_deg[i];
    float src_div_vol;
    float tgt_div_vol;
    float src_div_cut;
    float tgt_div_cut;

    if (!i_in_cmty) {
        src_div_vol = cmty_vol[cmty_idx] + i_deg;
        tgt_div_vol = cmty_vol[cmty_idx];
        src_div_cut = cmty_cut[cmty_idx] - 2 * deg_i_cmty + i_deg;
        tgt_div_cut = cmty_cut[cmty_idx];
    }
    else {
        src_div_vol = cmty_vol[cmty_idx];
        tgt_div_vol = cmty_vol[cmty_idx] - i_deg;
        src_div_cut = cmty_cut[cmty_idx];
        tgt_div_cut = cmty_cut[cmty_idx] + 2 * deg_i_cmty - i_deg;
    }

    const float tgt_div_se = (tgt_div_cut / graph_vol) * log(tgt_div_vol / graph_vol);
    const float src_div_se = (src_div_cut / graph_vol) * log(src_div_vol / graph_vol);
    const float node_i_se = (i_deg / graph_vol) * log(src_div_vol / graph_vol);
    const float src_tgt_se = (tgt_div_vol / graph_vol) * log(src_div_vol / tgt_div_vol);

    return tgt_div_se - src_div_se + node_i_se + src_tgt_se;
}

std::tuple<float, float, float, unsigned long, float, unsigned long, float>
CoDeSEG::_node_strategy(const NodeIdx i) {
    const auto src_cmty = node_cmty[i];
    auto tgt_cmty = src_cmty;

    auto cmty_in = std::unordered_map<unsigned long, float>{};
    const auto adj_i = graph_adj[i];
    for (const auto [j, w]: adj_i) {
        const auto div_j = node_cmty[j];
        cmty_in[div_j] = cmty_in[div_j] + w;
    }

    const float delta_leave_div_i = _delta_leave(i, src_cmty, cmty_in[src_cmty]);

    float delta_max = delta_leave_div_i;
    float delta_join_div_j = .0;
    for (const auto [cmty_j, cmty_j_i_in]: cmty_in) {
        if (cmty_j != src_cmty) {
            const auto delta_leave_div_j = _delta_leave(i, cmty_j, cmty_j_i_in, false);
            // const auto delta_trans_div_j = delta_leave_div_i - delta_leave_div_j;
            constexpr auto lambda = float{1.};
            // if (cmty_size[src_cmty] < 3 && cmty_size[cmty_j] >= 3)
            //     lambda = 0.1;

            const auto delta_trans_div_j = lambda * delta_leave_div_i - delta_leave_div_j;

            if (delta_trans_div_j > delta_max ||
                (delta_trans_div_j == delta_max && cmty_size[cmty_j] > cmty_size[tgt_cmty])) {
                delta_join_div_j = -delta_leave_div_j;
                delta_max = delta_trans_div_j;
                tgt_cmty = cmty_j;
            }
        }
    }

    return std::tuple{
        delta_max, delta_leave_div_i, delta_join_div_j,
        src_cmty, cmty_in[src_cmty], tgt_cmty, cmty_in[tgt_cmty]
    };
}

std::tuple<float, unsigned long> CoDeSEG::_ovlp_node(const NodeIdx i, const bool multi_thread) {
    const auto src_cmty = node_cmty[i];

    auto cmty_in = std::unordered_map<unsigned long, float>{};
    const auto adj_i = graph_adj[i];
    for (const auto [j, w]: adj_i) {
        const auto div_j = node_cmty[j];
        cmty_in[div_j] = cmty_in[div_j] + w;
    }

    float node_ovlp_se_sum = 0.;
    unsigned long node_ovlp_cmty_num = 0;
    for (const auto [cmty_j, cmty_j_i_in]: cmty_in) {
        if (cmty_j != src_cmty) {
            const auto delta_ovlp_cmty_j = -_delta_leave(i, cmty_j, cmty_j_i_in, false);
            const auto tau = static_cast<long double>(cmty_se_sum[cmty_j]) / cmty_size[cmty_j];
            if (delta_ovlp_cmty_j > tau) {
                node_ovlp_se_sum += delta_ovlp_cmty_j;
                node_ovlp_cmty_num += 1;

                if (multi_thread) {
                    std::unique_lock<std::mutex> lock(stat_mutex);
                    node_cmty_ovlp[i].insert(cmty_j);
                    // cmty_se_sum[cmty_j] += delta_ovlp_cmty_j;
                    // cmty_size[cmty_j] += 1;
                } else {
                    node_cmty_ovlp[i].insert(cmty_j);
                    // cmty_se_sum[cmty_j] += delta_ovlp_cmty_j;
                    // cmty_size[cmty_j] += 1;
                }
            }
        }
    }
    return std::tuple{node_ovlp_se_sum, node_ovlp_cmty_num};
}

void CoDeSEG::_transfer(const NodeIdx i,
                        const NodeIdx src_cmty, const float deg_i_src_cmty,
                        const NodeIdx tgt_cmty, const float deg_i_tgt_cmty) {
    const auto deg_i = node_deg[i];

    // update source community
    cmty_vol[src_cmty] -= deg_i;
    if (cmty_vol[src_cmty] > 0) {
        cmty_cut[src_cmty] += 2 * deg_i_src_cmty - deg_i;
    } else {
        cmty_cut[src_cmty] = 0.;
    }
    cmty_size[src_cmty] -= 1;

    // update target community
    node_cmty[i] = tgt_cmty;
    cmty_vol[tgt_cmty] += deg_i;
    cmty_cut[tgt_cmty] -= 2 * deg_i_tgt_cmty - deg_i;
    cmty_size[tgt_cmty] += 1;
}

CoDeSEG &CoDeSEG::detect_cmty(const unsigned int max_iter, const bool overlapping,
                              const bool verbose, const float se_threshold) {
    // variables for timing
    time_point<steady_clock> tm_start;

    if (verbose) tm_start = steady_clock::now();

    // initialize each node as an individual community
    _init_cluster();

    // community formation game main loop
    auto graph_se = graph_se_max;
    for (unsigned short it = 1; it < max_iter + 1; it++) {
        float delta_se_sum = 0.;
        unsigned long it_num_stay = 0;
        unsigned long it_num_trans = 0;

        for (std::size_t i = 0; i < nodes.size(); i++) {
            const auto [
                delta_se,
                delta_leave_div_i,
                delta_join_div_j,
                src_cmty, deg_i_src_cmty,
                tgt_cmty, deg_i_tgt_cmty
            ] = _node_strategy(i);

            if (cmty_version[tgt_cmty] < it) {
                cmty_se_sum[tgt_cmty] = 0.;
                cmty_version[tgt_cmty] = it;
            }

            if (delta_se > 0 && src_cmty != tgt_cmty) {
                delta_se_sum += delta_se;
                _transfer(i, src_cmty, deg_i_src_cmty, tgt_cmty, deg_i_tgt_cmty);
                it_num_trans += 1;

                cmty_se_sum[tgt_cmty] += delta_join_div_j;

                // show_communities(get_communities());
                // std::cout << delta_se << std::endl;
            } else {
                it_num_stay += 1;

                cmty_se_sum[src_cmty] += -delta_leave_div_i;
            }
        }

        graph_se -= delta_se_sum;

        if (verbose) {
            const auto duration = (std::chrono::steady_clock::now() - tm_start) / 1ms;

            std::cout << "{ iter: " << it << ", ";
            std::cout << "graph_se_max: " << graph_se_max << ", ";
            std::cout << "graph_se: " << graph_se << ", ";
            std::cout << "delta_se_sum: " << delta_se_sum << ", ";
            std::cout << "#stay: " << it_num_stay << ", ";
            std::cout << "#trans: " << it_num_trans << ", ";
            std::cout << "time: \"" << duration << "ms \"}" << std::endl;
        }

        // converged, break
        if (it_num_trans <= 0) break;

        if (delta_se_sum < se_threshold * graph_se_max) {
            std::cout << "converged!!!!" << std::endl;
            break;
        }
    }

    // overlap nodes
    if (overlapping) {
        // detect overlapping communities
        if (verbose) tm_start = steady_clock::now();

        float ovlp_se_sum = 0.;
        unsigned long ovlp_cmty_num = 0.;
        for (std::size_t i = 0; i < nodes.size(); i++) {
            node_cmty_ovlp.push(CmtyIdxSet());
            auto [node_ovlp_se_sum, node_ovlp_cmty_num] = _ovlp_node(i);

            ovlp_se_sum += node_ovlp_se_sum;
            ovlp_cmty_num += node_ovlp_cmty_num;
        }

        if (verbose) {
            const auto duration = (std::chrono::steady_clock::now() - tm_start) / 1ms;

            std::cout << "{ ovlp: ";
            std::cout << "ovlp_se_sum: " << ovlp_se_sum << ", ";
            std::cout << "#ovlp_cmty: " << ovlp_cmty_num << ", ";
            std::cout << "time: \"" << duration << "ms \"}" << std::endl;
        }
    }

    return *this;
}

CoDeSEG &CoDeSEG::detect_cmty(const unsigned int num_worker, const unsigned int max_iter,
                              const bool overlapping, const bool verbose, const float se_threshold) {
    // variables for timing
    time_point<steady_clock> tm_start;

    if (verbose) tm_start = steady_clock::now();

    // initialize each node as individal community
    _init_cluster();

    // compute number of nodes per block
    auto num_per_blk = nodes.size() / num_worker + 1;

    // initialize a thread pool
    ThreadPool pool(num_worker);
    std::vector<TaskReturn> results;
    results.reserve(num_worker);

    stats_mutex.clear();
    for (auto blk = 0; blk < num_worker; blk++) {
        stats_mutex.emplace_back(new std::mutex);
    }

    // community formation game main loop
    auto graph_se = graph_se_max;
    for (unsigned short it = 1; it < max_iter + 1; it++) {
        float delta_se_sum = 0.;
        unsigned long it_num_stay = 0;
        unsigned long it_num_trans = 0;
        // std::cout << "iteration: " << it << std::endl;

        for (auto blk = 0; blk < num_worker; blk++) {
            auto idx_start = std::min(blk * num_per_blk, nodes.size());
            auto idx_end = std::min(blk * num_per_blk + num_per_blk, nodes.size());

            auto ptr_inst = this;
            results.emplace_back(
                pool.enqueue([idx_start, idx_end, it, ptr_inst, num_per_blk] {
                    float tsk_delta_se_sum = 0.;
                    unsigned long tsk_num_stay = 0;
                    unsigned long tsk_num_trans = 0;

                    for (std::size_t i = idx_start; i < idx_end; i++) {
                        const auto [
                            delta_se,
                            delta_leave_div_i,
                            delta_join_div_j,
                            src_cmty, deg_i_src_cmty,
                            tgt_cmty, deg_i_tgt_cmty
                        ] = ptr_inst->_node_strategy(i);

                        std::unique_lock lock(ptr_inst->stat_mutex);

                        auto real_deg_i_src_cmty = float{0.};
                        auto real_deg_i_tgt_cmty = float{0.};
                        if (delta_se > 0 && src_cmty != tgt_cmty && ptr_inst->cmty_size[tgt_cmty] > 0) {
                            const auto adj_i = ptr_inst->graph_adj[i];
                            for (const auto [j, w]: adj_i) {
                                const auto div_j = ptr_inst->node_cmty[j];
                                if (div_j == tgt_cmty) {
                                    real_deg_i_tgt_cmty += w;
                                } else if (div_j == src_cmty) {
                                    real_deg_i_src_cmty += w;
                                }
                            }

                            const auto real_delta_leave_src_div =
                                    ptr_inst->_delta_leave(i, src_cmty, real_deg_i_src_cmty);
                            const auto real_delta_leave_tgt_div =
                                    ptr_inst->_delta_leave(i, tgt_cmty, real_deg_i_tgt_cmty, false);

                            const auto real_delta = real_delta_leave_src_div - real_delta_leave_tgt_div;

                            if (real_delta > 0) {
                                tsk_delta_se_sum += real_delta;

                                ptr_inst->_transfer(i, src_cmty, real_deg_i_src_cmty,
                                                    tgt_cmty, real_deg_i_tgt_cmty);
                                tsk_num_trans += 1;
                                ptr_inst->cmty_se_sum[tgt_cmty] += -real_delta_leave_tgt_div;
                            } else {
                                tsk_num_stay += 1;
                                ptr_inst->cmty_se_sum[src_cmty] += -real_delta_leave_src_div;
                            }

                            // show_communities(get_communities());
                            // std::cout << delta_se << std::endl;
                        } else {
                            tsk_num_stay += 1;
                            ptr_inst->cmty_se_sum[src_cmty] += -delta_leave_div_i;
                        }
                    }


                    return std::tuple(tsk_delta_se_sum, tsk_num_trans, tsk_num_stay);
                })
            );
        }

        for (auto &&result: results) {
            auto [tsk_delta_se_sum, tsk_num_trans, tsk_num_stay] = result.get();
            // std::cout << "done: SE: " << tsk_delta_se_sum << ", #num_trans: " << tsk_num_trans
            //         << ", #num_stay: " << tsk_num_stay << std::endl;

            delta_se_sum += tsk_delta_se_sum;
            it_num_stay += tsk_num_stay;
            it_num_trans += tsk_num_trans;
        }

        results.clear();

        graph_se -= delta_se_sum;

        if (verbose) {
            const auto duration = (std::chrono::steady_clock::now() - tm_start) / 1ms;

            std::cout << "{ iter: " << it << ", ";
            std::cout << "graph_se_max: " << graph_se_max << ", ";
            std::cout << "graph_se: " << graph_se << ", ";
            std::cout << "delta_se_sum: " << delta_se_sum << ", ";
            std::cout << "#stay: " << it_num_stay << ", ";
            std::cout << "#trans: " << it_num_trans << ", ";
            std::cout << "time: \"" << duration << "ms\" }" << std::endl;
        }

        // converged, break
        if (it_num_trans <= 0) break;

        if (delta_se_sum < se_threshold * graph_se_max) {
            std::cout << "converged!!!!" << std::endl;
            break;
        }
    }

    // overlap nodes
    if (overlapping) {
        // detect overlapping communities
        if (verbose) tm_start = steady_clock::now();

        float ovlp_se_sum = 0.;
        unsigned long ovlp_cmty_num = 0.;

        for (auto blk = 0; blk < num_worker; blk++) {
            auto idx_start = std::min(blk * num_per_blk, nodes.size());
            auto idx_end = std::min(blk * num_per_blk + num_per_blk, nodes.size());

            for (std::size_t i = idx_start; i < idx_end; i++) {
                node_cmty_ovlp.push(CmtyIdxSet());
            }

            auto ptr_inst = this;
            results.emplace_back(
                pool.enqueue([idx_start, idx_end, ptr_inst] {
                    // std::cout << "processing, start: " << idx_start << ", end: " << idx_end << std::endl;
                    // std::this_thread::sleep_for(std::chrono::seconds(3));
                    // return std::tuple(idx_start, idx_end);

                    float tsk_ovlp_se_sum = 0.;
                    unsigned long tsk_ovlp_cmty_num = 0.;
                    for (std::size_t i = idx_start; i < idx_end; i++) {
                        // ptr_inst->node_cmty_ovlp.push(CmtyIdxSet());
                        auto [
                            node_ovlp_se_sum,
                            node_ovlp_cmty_num
                        ] = ptr_inst->_ovlp_node(i, true);

                        std::unique_lock<std::mutex> lock(ptr_inst->stat_mutex);
                        tsk_ovlp_se_sum += node_ovlp_se_sum;
                        tsk_ovlp_cmty_num += node_ovlp_cmty_num;
                    }

                    return std::tuple(tsk_ovlp_se_sum, tsk_ovlp_cmty_num, static_cast<unsigned long>(0));
                })
            );
        }

        for (auto &&result: results) {
            auto [tsk_delta_se_sum, tsk_num_ovlp, _] = result.get();
            // std::cout << "done: SE: " << tsk_delta_se_sum << ", #num_ovlp: " << tsk_num_ovlp << std::endl;

            ovlp_se_sum += tsk_delta_se_sum;
            ovlp_cmty_num += tsk_num_ovlp;
        }

        results.clear();

        if (verbose) {
            const auto duration = (std::chrono::steady_clock::now() - tm_start) / 1ms;

            std::cout << "{ ovlp: ";
            std::cout << "ovlp_se_sum: " << ovlp_se_sum << ", ";
            std::cout << "#ovlp_cmty: " << ovlp_cmty_num << ", ";
            std::cout << "time: \"" << duration << "ms\" }" << std::endl;
        }
    }

    return *this;
}

CmtyMap &CoDeSEG::communities() {
    cmtis.clear();

    for (std::size_t i = 0; i < node_cmty.size(); i++) {
        const auto node_label = node_cmty[i];
        const auto node = nodes[i];

        cmtis[node_label].insert(node);
    }

    if (node_cmty_ovlp.size() > 0) {
        for (std::size_t i = 0; i < node_cmty.size(); i++) {
            const auto node = nodes[i];
            for (const auto &ovlp_cmty: node_cmty_ovlp[i]) {
                cmtis[ovlp_cmty].insert(node);
            }
        }
    }

    return cmtis;
}
