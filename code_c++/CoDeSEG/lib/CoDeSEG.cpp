#include "CoDeSEG.h"
#include <cmath>
#include <iostream>

#include "ThreadPool.h"

NodeIdx CoDeSEG::_add_or_get_node(const Node &node, const bool direct) {
    const auto el = node_idx.find(node);

    if (el == node_idx.end()) {
        const auto idx = node_idx.size();
        node_idx[node] = idx;
        nodes.push(node);
        graph_adj.push(NodeAdj());
        if (direct) graph_adj_out.push(NodeAdj());
        return idx;
    }
    return el->second;
}

CoDeSEG &CoDeSEG::add_edges(EdgeArray &edges, const bool direct) {
    for (unsigned long i = 0; i < edges.size(); i++) {
        auto &[src, tgt, weight] = edges[i];
        add_edge(src, tgt, weight, direct);
    }
    return *this;
}

CoDeSEG &CoDeSEG::add_edge(const Node &src, const Node &tgt, const float weight, const bool direct) {
    const auto src_idx = _add_or_get_node(src, direct);
    const auto tgt_idx = _add_or_get_node(tgt, direct);

    graph_adj[tgt_idx][src_idx] = weight;
    if(direct){ 
        graph_adj_out[src_idx][tgt_idx] = weight;
    }
    else{
        graph_adj[src_idx][tgt_idx] = weight;
    }



    return *this;
}


void CoDeSEG::_init_cluster() {
    // 计算每节点的度(边权重之和)\图节点度之和(volume)
    graph_vol = 0.;
    for (size_t i = 0; i < nodes.size(); i++) {
        // 从邻接节点计算节点
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

    for (size_t i = 0; i < nodes.size(); i++) {
        const auto p = node_deg[i]/graph_vol;
        se_1d -= p*log(p);
    }

    avg_se_1d = se_1d/nodes.size();

    // 初始化
    cmty_vol.copy_from(node_deg);
    cmty_cut.copy_from(node_deg);
}




float CoDeSEG::_delta_leave(const NodeIdx i, const NodeIdx cmty_idx, const FloatValue deg_i_cmty,
                            const bool i_in_cmty) {
    // 单一结点社区
    if (i_in_cmty && std::abs(cmty_vol[cmty_idx] - node_deg[i]) < 1e-8) return 0.;

    const auto i_deg = node_deg[i];
    float src_div_vol;
    float tgt_div_vol;
    float src_div_cut;
    float tgt_div_cut;

    // 节点 i 不在社区 cmty_idx
    if (!i_in_cmty) {
        src_div_vol = cmty_vol[cmty_idx] + i_deg;
        tgt_div_vol = cmty_vol[cmty_idx];
        src_div_cut = cmty_cut[cmty_idx] - 2 * deg_i_cmty + i_deg;
        tgt_div_cut = cmty_cut[cmty_idx];
    }
    // 节点 i 在社区 cmty_idx
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

            if (delta_trans_div_j > delta_max) {
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

std::tuple<float, unsigned long> CoDeSEG::_ovlp_node(const NodeIdx i, const float alpha, const bool multi_thread) {
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
            const auto tau = static_cast<long double>(cmty_se_sum[cmty_j]) / cmty_size[cmty_j] * alpha;
            if (delta_ovlp_cmty_j > tau) {
                node_ovlp_se_sum += delta_ovlp_cmty_j;
                node_ovlp_cmty_num += 1;

                if (multi_thread) {
                    std::unique_lock<std::mutex> lock(stat_mutex);
                    node_cmty_ovlp[i].insert(cmty_j);
                    cmty_se_sum[cmty_j] += delta_ovlp_cmty_j;
                    cmty_size[cmty_j] += 1;
                } else {
                    node_cmty_ovlp[i].insert(cmty_j);
                    cmty_se_sum[cmty_j] += delta_ovlp_cmty_j;
                    cmty_size[cmty_j] += 1;
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

CoDeSEG &CoDeSEG::detect_cmty(const unsigned int max_iter, const bool overlapping, const float tau, const float alpha, const bool verbose) {
    time_t tm_start, tm_finish;

    if (verbose) time(&tm_start);

    // initialize each node as individal community
    _init_cluster();

    // community formation game main loop
    auto leave_se = se_1d; 
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

        leave_se -= delta_se_sum;
        if (verbose) {
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ iter: " << it << ", ";
            std::cout << "delta_se_sum: " << delta_se_sum << ", ";
            std::cout << "leave_se: " << leave_se << ", ";
            std::cout << "se_sum: " << se_1d << ", ";
            std::cout << "avg_se_sum: " << avg_se_1d << ", ";
            std::cout << "#stay: " << it_num_stay << ", ";
            std::cout << "#trans: " << it_num_trans << ", ";
            std::cout << "time: \"" << tm_diff << " sec \"}" << std::endl;
        }

        // converged, break
        if (it_num_trans <= 0 ) break;
        // if ( delta_se_sum <= tau * se_1d) break;
        if ( delta_se_sum/it_num_trans <= tau * avg_se_1d) break;
    }

    // overlap nodes
    if (overlapping) {
        // detect overlapping communities
        if (verbose) time(&tm_start);


        float ovlp_se_sum = 0.;
        unsigned long ovlp_cmty_num = 0.;
        for (std::size_t i = 0; i < nodes.size(); i++) {
            node_cmty_ovlp.push(CmtyIdxSet());
            auto [node_ovlp_se_sum, node_ovlp_cmty_num] = _ovlp_node(i, alpha);

            ovlp_se_sum += node_ovlp_se_sum;
            ovlp_cmty_num += node_ovlp_cmty_num;
        }

        if (verbose) {
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ ovlp: ";
            std::cout << "ovlp_se_sum: " << ovlp_se_sum << ", ";
            std::cout << "#ovlp_cmty: " << ovlp_cmty_num << ", ";
            std::cout << "time: \"" << tm_diff << " sec \"}" << std::endl;
        }
    }

    return *this;
}

CoDeSEG &CoDeSEG::detect_cmty(const unsigned int num_worker, const unsigned int max_iter,
                              const bool overlapping, const float tau, const float alpha, const bool verbose) {
    // variables for timing
    time_t tm_start, tm_finish;

    if (verbose) time(&tm_start);

    // initialize each node as individal community
    _init_cluster();

    // compute number of nodes per block
    auto num_per_blk = nodes.size() / num_worker + 1;

    // initialize a thread pool
    ThreadPool pool(num_worker);
    std::vector<TaskReturn> results;
    results.reserve(num_worker);

    // community formation game main loop
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
                pool.enqueue([idx_start, idx_end, it, ptr_inst] {
                    // std::cout << "processing, start: " << idx_start << ", end: " << idx_end << std::endl;
                    // std::this_thread::sleep_for(std::chrono::seconds(3));
                    // return std::tuple(idx_start, idx_end);

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

                        std::unique_lock<std::mutex> lock(ptr_inst->stat_mutex);

                        if (ptr_inst->cmty_version[tgt_cmty] < it) {
                            ptr_inst->cmty_se_sum[tgt_cmty] = 0.;
                            ptr_inst->cmty_version[tgt_cmty] = it;
                        }

                        // 解决线程冲突:
                        // 1）目标社区不存在 -> stay
                        // 2) 源社区和目标社区可能发生变化 -> 重新计算 -> tranfer or stay
                        auto real_deg_i_src_cmty = float{0.};
                        auto real_deg_i_tgt_cmty = float{0.};
                        if (delta_se > 0 && src_cmty != tgt_cmty && ptr_inst->cmty_size[tgt_cmty] > 0) {
                            // 重新计算 delta, deg_i
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

         if (verbose) {
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ iter: " << it << ", ";
            std::cout << "delta_se_sum: " << delta_se_sum << ", ";
            std::cout << "se_sum: " << se_1d << ", ";
            std::cout << "avg_se_sum: " << avg_se_1d << ", ";
            std::cout << "#stay: " << it_num_stay << ", ";
            std::cout << "#trans: " << it_num_trans << ", ";
            std::cout << "time: \"" << tm_diff << " sec \"}" << std::endl;
        }

        // converged, break
        if (it_num_trans <= 0 ) break;
        // if ( delta_se_sum <= tau * se_1d) break;
        if ( delta_se_sum/it_num_trans <= tau * avg_se_1d) break;
    }

    // overlap nodes
    if (overlapping) {
        // detect overlapping communities
        if (verbose) time(&tm_start);

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
                pool.enqueue([idx_start, idx_end, ptr_inst,alpha] {
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
                        ] = ptr_inst->_ovlp_node(i, alpha, true);

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
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ ovlp: ";
            std::cout << "ovlp_se_sum: " << ovlp_se_sum << ", ";
            std::cout << "#ovlp_cmty: " << ovlp_cmty_num << ", ";
            std::cout << "time: \"" << tm_diff << "sec\" }" << std::endl;
        }
    }

    return *this;
}

std::tuple<float, float, float, unsigned long, float, float, unsigned long, float, float>
CoDeSEG::_direct_node_strategy(const NodeIdx i) {
    const auto src_cmty = node_cmty[i];
    auto tgt_cmty = src_cmty;

    auto cmty_in = std::unordered_map<NodeIdx, std::pair<float, float>>{};
    const auto adj_i = graph_adj[i];
    for (const auto [j, w]: adj_i) {
        const auto div_j = node_cmty[j];
        cmty_in[div_j].first +=  w;
    }
    for (const auto [k, w]: graph_adj_out[i]) {
            const auto div_k = node_cmty[k];

            if(cmty_in.find(div_k) != cmty_in.end()) cmty_in[div_k].second += w;
        }

    const float delta_leave_div_i = _delta_leave(i, src_cmty, cmty_in[src_cmty].first, cmty_in[src_cmty].second);

    float delta_max = delta_leave_div_i;
    float delta_join_div_j = .0;
    for (const auto [cmty_j, pair_value]: cmty_in) {
        if (cmty_j != src_cmty) {
            const auto& [t_in, t_out] = pair_value;
            const auto delta_leave_div_j = _direct_delta_leave(i, cmty_j, t_in, t_out, false);
            // const auto delta_trans_div_j = delta_leave_div_i - delta_leave_div_j;
            constexpr auto lambda = float{1.};
            // if (cmty_size[src_cmty] < 3 && cmty_size[cmty_j] >= 3)
            //     lambda = 0.1;

            const auto delta_trans_div_j = lambda * delta_leave_div_i - delta_leave_div_j;

            if (delta_trans_div_j > delta_max) {
                delta_join_div_j = -delta_leave_div_j;
                delta_max = delta_trans_div_j;
                tgt_cmty = cmty_j;
            }
        }
    }

    return std::tuple{
        delta_max, delta_leave_div_i, delta_join_div_j,
        src_cmty, cmty_in[src_cmty].first, cmty_in[src_cmty].second, tgt_cmty, cmty_in[tgt_cmty].first, cmty_in[tgt_cmty].second
    };
}

float CoDeSEG::_direct_delta_leave(const NodeIdx i, const NodeIdx cmty_idx, const FloatValue deg_i_cmty, const FloatValue deg_i_cmty_out, const bool i_in_cmty) {
    // 单一结点社区
    if (i_in_cmty && std::abs(cmty_vol[cmty_idx] - node_deg[i]) < 1e-8) return 0.;

    const auto i_deg = node_deg[i];
    float src_div_vol;
    float tgt_div_vol;
    float src_div_cut;
    float tgt_div_cut;

    // 节点 i 不在社区 cmty_idx
    if (!i_in_cmty) {
        src_div_vol = cmty_vol[cmty_idx] + i_deg;
        tgt_div_vol = cmty_vol[cmty_idx];
        src_div_cut = cmty_cut[cmty_idx] - deg_i_cmty - deg_i_cmty_out + i_deg;
        tgt_div_cut = cmty_cut[cmty_idx];
    }
    // 节点 i 在社区 cmty_idx
    else {
        src_div_vol = cmty_vol[cmty_idx];
        tgt_div_vol = cmty_vol[cmty_idx] - i_deg;
        src_div_cut = cmty_cut[cmty_idx];
        tgt_div_cut = cmty_cut[cmty_idx] + deg_i_cmty + deg_i_cmty_out - i_deg;
    }

    const float tgt_div_se = (tgt_div_cut / graph_vol) * log(tgt_div_vol / graph_vol);
    const float src_div_se = (src_div_cut / graph_vol) * log(src_div_vol / graph_vol);
    const float node_i_se = (i_deg / graph_vol) * log(src_div_vol / graph_vol);
    const float src_tgt_se = (tgt_div_vol / graph_vol) * log(src_div_vol / tgt_div_vol);

    return tgt_div_se - src_div_se + node_i_se + src_tgt_se;
}

void CoDeSEG::_direct_transfer(NodeIdx i,
                   NodeIdx src_cmty, float deg_i_src_cmty_in, float deg_i_src_cmty_out,
                   NodeIdx tgt_cmty, float deg_i_tgt_cmty_in, float deg_i_tgt_cmty_out) {
    const auto deg_i = node_deg[i];

    // update source community
    cmty_vol[src_cmty] -= deg_i;
    if (cmty_vol[src_cmty] > 0) {
        cmty_cut[src_cmty] += deg_i_src_cmty_in + deg_i_src_cmty_out - deg_i;
    } else {
        cmty_cut[src_cmty] = 0.;
    }
    cmty_size[src_cmty] -= 1;

    // update target community
    node_cmty[i] = tgt_cmty;
    cmty_vol[tgt_cmty] += deg_i;
    cmty_cut[tgt_cmty] -= deg_i_tgt_cmty_in + deg_i_tgt_cmty_out - deg_i;
    cmty_size[tgt_cmty] += 1;
}

std::tuple<float, unsigned long> CoDeSEG::_direct_ovlp_node(const NodeIdx i, const float alpha, const bool multi_thread) {
    const auto src_cmty = node_cmty[i];

    auto cmty_in = std::unordered_map<NodeIdx, std::pair<float, float>>{};
    const auto adj_i = graph_adj[i];
    for (const auto [j, w]: adj_i) {
        const auto div_j = node_cmty[j];
        cmty_in[div_j].first +=  w;
    }

    for (const auto [k, w]: graph_adj_out[i]) {
            const auto div_k = node_cmty[k];

            if(cmty_in.find(div_k) != cmty_in.end()) cmty_in[div_k].second += w;
        }

    float node_ovlp_se_sum = 0.;
    unsigned long node_ovlp_cmty_num = 0;
    for (const auto [cmty_j, pair_value]: cmty_in) {
       
        if (cmty_j != src_cmty) {
            const auto& [t_in, t_out] = pair_value;
            const auto delta_ovlp_cmty_j = -_direct_delta_leave(i, cmty_j, t_in, t_out, false);
            const auto tau = static_cast<long double>(cmty_se_sum[cmty_j]) / cmty_size[cmty_j] * alpha;

            if (delta_ovlp_cmty_j > tau) {
                node_ovlp_se_sum += delta_ovlp_cmty_j;
                node_ovlp_cmty_num += 1;

                if (multi_thread) {
                    std::unique_lock<std::mutex> lock(stat_mutex);
                    node_cmty_ovlp[i].insert(cmty_j);
                    cmty_se_sum[cmty_j] += delta_ovlp_cmty_j;
                    cmty_size[cmty_j] += 1;
                } else {
                    node_cmty_ovlp[i].insert(cmty_j);
                    cmty_se_sum[cmty_j] += delta_ovlp_cmty_j;
                    cmty_size[cmty_j] += 1;
                }
            }
            
           
        }
    }
    return std::tuple{node_ovlp_se_sum, node_ovlp_cmty_num};
}

CoDeSEG &CoDeSEG::detect_cmty_direct(const unsigned int max_iter, const bool overlapping, const float tau, const float alpha, const bool verbose) {
    time_t tm_start, tm_finish;
    

    if (verbose) time(&tm_start);

    // initialize each node as individal community
    _init_cluster();

    // community formation game main loop
    for (unsigned short it = 1; it < max_iter + 1; it++) {
        float delta_se_sum = 0.;
        unsigned long it_num_stay = 0;
        unsigned long it_num_trans = 0;
        for (std::size_t i = 0; i < nodes.size(); i++) {
            const auto [
                delta_se,
                delta_leave_div_i,
                delta_join_div_j,
                src_cmty, deg_i_src_cmty_in, deg_i_src_cmty_out,
                tgt_cmty, deg_i_tgt_cmty_in, deg_i_tgt_cmty_out
            ] = _direct_node_strategy(i);

            if (cmty_version[tgt_cmty] < it) {
                cmty_se_sum[tgt_cmty] = 0.;
                cmty_version[tgt_cmty] = it;
            }

            if (delta_se > 0 && src_cmty != tgt_cmty) {
                delta_se_sum += delta_se;
                _direct_transfer(i, src_cmty, deg_i_src_cmty_in, deg_i_src_cmty_out, tgt_cmty, deg_i_tgt_cmty_in, deg_i_tgt_cmty_out);
                it_num_trans += 1;

                cmty_se_sum[tgt_cmty] += delta_join_div_j;

                // show_communities(get_communities());
                // std::cout << delta_se << std::endl;
            } else {
                it_num_stay += 1;

                cmty_se_sum[src_cmty] += -delta_leave_div_i;
            }
        }
      

         if (verbose) {
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ iter: " << it << ", ";
            std::cout << "delta_se_sum: " << delta_se_sum << ", ";
            std::cout << "se_sum: " << se_1d << ", ";
            std::cout << "avg_se_sum: " << avg_se_1d << ", ";
            std::cout << "#stay: " << it_num_stay << ", ";
            std::cout << "#trans: " << it_num_trans << ", ";
            std::cout << "time: \"" << tm_diff << " sec \"}" << std::endl;
        }

        // converged, break
        if (it_num_trans <= 0 ) break;
        // if ( delta_se_sum <= tau * se_1d) break;
        if ( delta_se_sum/it_num_trans <= tau * avg_se_1d) break;
    }

   // overlap nodes
    if (overlapping) {
        if (verbose) time(&tm_start);

        float ovlp_se_sum = 0.;
        unsigned long ovlp_cmty_num = 0.;
        for (std::size_t i = 0; i < nodes.size(); i++) {
            node_cmty_ovlp.push(CmtyIdxSet());
            auto [node_ovlp_se_sum, node_ovlp_cmty_num] = _direct_ovlp_node(i, alpha);

            ovlp_se_sum += node_ovlp_se_sum;
            ovlp_cmty_num += node_ovlp_cmty_num;
        }

        if (verbose) {
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ ovlp: ";
            std::cout << "ovlp_se_sum: " << ovlp_se_sum << ", ";
            std::cout << "#ovlp_cmty: " << ovlp_cmty_num << ", ";
            std::cout << "time: \"" << tm_diff << " sec \"}" << std::endl;
        }
    }

    return *this;
}

CoDeSEG &CoDeSEG::detect_cmty_direct(const unsigned int num_worker, const unsigned int max_iter,
                              const bool overlapping, const float tau, const float alpha, const bool verbose) {
    // variables for timing
    time_t tm_start, tm_finish;

    if (verbose) time(&tm_start);

    // initialize each node as individal community
    _init_cluster();

    // compute number of nodes per block
    auto num_per_blk = nodes.size() / num_worker + 1;

    // initialize a thread pool
    ThreadPool pool(num_worker);
    std::vector<TaskReturn> results;
    results.reserve(num_worker);

    // community formation game main loop
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
                pool.enqueue([idx_start, idx_end, it, ptr_inst] {
                    // std::cout << "processing, start: " << idx_start << ", end: " << idx_end << std::endl;
                    // std::this_thread::sleep_for(std::chrono::seconds(3));
                    // return std::tuple(idx_start, idx_end);

                    float tsk_delta_se_sum = 0.;
                    unsigned long tsk_num_stay = 0;
                    unsigned long tsk_num_trans = 0;

                    for (std::size_t i = idx_start; i < idx_end; i++) {
                        const auto [
                            delta_se,
                            delta_leave_div_i,
                            delta_join_div_j,
                            src_cmty, deg_i_src_cmty_in, deg_i_src_cmty_out,
                            tgt_cmty, deg_i_tgt_cmty_in, deg_i_tgt_cmty_out
                        ] = ptr_inst->_direct_node_strategy(i);

                        std::unique_lock<std::mutex> lock(ptr_inst->stat_mutex);

                        if (ptr_inst->cmty_version[tgt_cmty] < it) {
                            ptr_inst->cmty_se_sum[tgt_cmty] = 0.;
                            ptr_inst->cmty_version[tgt_cmty] = it;
                        }

                        // 解决线程冲突:
                        // 1）目标社区不存在 -> stay
                        // 2) 源社区和目标社区可能发生变化 -> 重新计算 -> tranfer or stay
                        auto real_deg_i_src_cmty = float{0.};
                        auto real_deg_i_src_cmty_out = float{0.};
                        auto real_deg_i_tgt_cmty = float{0.};
                        auto real_deg_i_tgt_cmty_out = float{0.};
                        if (delta_se > 0 && src_cmty != tgt_cmty && ptr_inst->cmty_size[tgt_cmty] > 0) {
                            // 重新计算 delta, deg_i
                            const auto adj_i = ptr_inst->graph_adj[i];
                            for (const auto [j, w]: adj_i) {
                                const auto div_j = ptr_inst->node_cmty[j];
                                if (div_j == tgt_cmty) {
                                    real_deg_i_tgt_cmty += w;
                                } else if (div_j == src_cmty) {
                                    real_deg_i_src_cmty += w;
                                }
                            }

                            const auto adj_i_out = ptr_inst->graph_adj_out[i];
                            for (const auto [j, w]: adj_i) {
                                const auto div_j = ptr_inst->node_cmty[j];
                                if (div_j == tgt_cmty) {
                                    real_deg_i_tgt_cmty_out += w;
                                } else if (div_j == src_cmty) {
                                    real_deg_i_src_cmty_out += w;
                                }
                            }

                            const auto real_delta_leave_src_div =
                                    ptr_inst->_direct_delta_leave(i, src_cmty, real_deg_i_src_cmty, real_deg_i_src_cmty_out);
                            const auto real_delta_leave_tgt_div =
                                    ptr_inst->_direct_delta_leave(i, tgt_cmty, real_deg_i_tgt_cmty, real_deg_i_tgt_cmty_out, false);

                            const auto real_delta = real_delta_leave_src_div - real_delta_leave_tgt_div;

                            if (real_delta > 0) {
                                tsk_delta_se_sum += real_delta;

                                ptr_inst->_direct_transfer(i, src_cmty, real_deg_i_src_cmty, real_deg_i_src_cmty_out,
                                                    tgt_cmty, real_deg_i_tgt_cmty, real_deg_i_tgt_cmty_out);
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

         if (verbose) {
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ iter: " << it << ", ";
            std::cout << "delta_se_sum: " << delta_se_sum << ", ";
            std::cout << "se_sum: " << se_1d << ", ";
            std::cout << "avg_se_sum: " << avg_se_1d << ", ";
            std::cout << "#stay: " << it_num_stay << ", ";
            std::cout << "#trans: " << it_num_trans << ", ";
            std::cout << "time: \"" << tm_diff << " sec \"}" << std::endl;
        }

        // converged, break
        if (it_num_trans <= 0 ) break;
        // if ( delta_se_sum <= tau * se_1d) break;
        if ( delta_se_sum/it_num_trans <= tau * avg_se_1d) break;
    }

    // overlap nodes
    if (overlapping) {
        // detect overlapping communities
        if (verbose) time(&tm_start);

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
                pool.enqueue([idx_start, idx_end, ptr_inst, alpha] {
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
                        ] = ptr_inst->_direct_ovlp_node(i, alpha, true);

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
            time(&tm_finish);
            const auto tm_diff = difftime(tm_finish, tm_start);

            std::cout << "{ ovlp: ";
            std::cout << "ovlp_se_sum: " << ovlp_se_sum << ", ";
            std::cout << "#ovlp_cmty: " << ovlp_cmty_num << ", ";
            std::cout << "time: \"" << tm_diff << "sec\" }" << std::endl;
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
