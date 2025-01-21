# Community Detection in Large-Scale Complex Networks via Structural Entropy Game (CoDeSEG)
Notably, if some folder cannot be opened, please download the entire repository.
# Codes
## CoDeSEG (ours)
step1. if the executable file under the code_c++\CoDeSEG\build path is not available, please repackaging according to the Makefile.

step2. update the executable file path in code_py\CoDeSEG.py.

step3. run code_py\CoDeSEG.py.

## Bigclam [1]
step1. download the code from bigclam from [https://github.com/snap-stanford/snap](https://github.com/snap-stanford/snap) and package it as an executable file.

step2. update the executable file path in code_py\bigclam.py.

step3. run code_py\bigclam.py.

## SLPA [2]
step1. run code_py\SLPA.py.

We implement this algorithm in Python3.8 with the NetworkX package in CDlib, with the number of iterations set to 21 and the filtering threshold set to 0.01. 
The original code is available at [https://github.com/kbalasu/SLPA](https://github.com/kbalasu/SLPA).

## NcGame [3]
step1. run code_py\NCG.py.

We implement this algorithm with python3.8 via open-source code available in the paper’s supplementary material [https://doi.org/10.1038/s41598-022-15095-9](https://doi.org/10.1038/s41598-022-15095-9).

## Fox [4]
step1. if the executable file under the code_c++\fox path is not available, please repackaging according to the Makefile. 

step2. update the executable file path in code_py\lazyfox.py.

step3. run code_py\exp_fox.py.

We implement this algorithm through the open source code provided in the paper [5].

## Louvain [6]
step1. run code_py\louvain.py. 

We implement this algorithm in Python3.8 with API provided by igraph library, and its open-source code is available at [https://github.com/taynaud/python-louvain](https://github.com/taynaud/python-louvain).

## DER [7]
step1. run code_py\DER_impt.py. 

We implement this algorithm in Python3.8 with the NetworkX package in CDlib, and its original code is available at [https://github.com/komarkdev/der_graph_clustering](https://github.com/komarkdev/der_graph_clustering).

## Leiden [8]
step1. run code_py\leiden.py. 

We implement this algorithm using open-source code available at [https://github.com/vtraag/leidenalg](https://github.com/vtraag/leidenalg).

## FLPA [9]
step1. run code_py\FLPA_impt.py. 

We implement this algorithm in Python3.8 with API provided by igraph library, and its open-source code is available at [https://github.com/vtraag/igraph/tree/flpa](https://github.com/vtraag/igraph/tree/flpa).

# Datasets
Please update the file paths for all networks in code_py\comm_utils.py

## Overlapping Networks [10]
For Amazon，YouTube, DBLP, LiveJournal, Orkut, Friendster, and Wiki datasets, which can be download at [https://snap.stanford.edu/data](https://snap.stanford.edu/data).
If you want to convert Wiki into an undirected graph, please execute code_py\wiki_to_undirect.py

## Non-Overlapping Networks
The tweet12 [11] dataset contains 68,841 annotated English tweets covering 503 different event categories, encompassing tweets over a consecutive 29-day period.
The tweet18 [12] includes 64,516 annotated French tweets covering 257 different event categories, with data spanning over a consecutive 23-day period. 
If using these two datasets, execute code_py\tweet_preprocess.py to convert them into networks.

# Evaluation metrics
## F1 and NMI
step1. download the code from bigclam from [https://github.com/eXascaleInfolab/xmeasures](https://github.com/eXascaleInfolab/xmeasures) and package it as an executable file.

step2. update the executable file path in code_py\xmeasures.py.

## ONMI
step1. download the code from bigclam from [https://github.com/eXascaleInfolab/OvpNMI](https://github.com/eXascaleInfolab/OvpNMI) and package it as an executable file.

step2. update the executable file path in code_py\omni.py.

# Citation
If you find this repository helpful, please consider citing the following paper.

[1] Jaewon Yang and Jure Leskovec. 2013. Overlapping community detection at scale: a nonnegative matrix factorization approach. In Proceedings of the sixth ACM international conference on Web search and data mining. 587–596.

[2] Jierui Xie, Boleslaw K Szymanski, and Xiaoming Liu. 2011. Slpa: Uncoverin overlapping communities in social networks via a speaker-listener interaction dynamic process. In 2011 ieee 11th international conference on data mining workshops. IEEE, 344–349.

[3]Farhad Ferdowsi and Keivan Aghababaei Samani. 2022. Detecting overlapping communities in complex networks using non-cooperative games. Scientific Reports 12, 1 (2022), 11054.

[4] Tianshu Lyu, Lidong Bing, Zhao Zhang, and Yan Zhang. 2020. Fox: fast overlapping community detection algorithm in big weighted networks. ACM Transactions on Social Computing 3, 3 (2020), 1–23.

[5] Tim Garrels, Athar Khodabakhsh, Bernhard Y Renard, and Katharina Baum. 2023. LazyFox: Fast and parallelized overlapping community detection in large graphs. PeerJ Computer Science 9 (2023), e1291.

[6] Vincent D Blondel, Jean-Loup Guillaume, Renaud Lambiotte, and Etienne Lefebvre. 2008. Fast unfolding of communities in large networks. Journal of statistical mechanics: theory and experiment 2008, 10 (2008), P10008.

[7] Mark Kozdoba and Shie Mannor. 2015. Community detection via measure space embedding. In Proceedings of the 28th International Conference on Neural Information Processing Systems-Volume 2. 2890–2898.

[8] VA Traag, L Waltman, and NJ van Eck. 2019. From Louvain to Leiden: guaranteeing well-connected communities. Scientific Reports 9 (2019), 5233.

[9] Vincent A Traag and Lovro Šubelj. 2023. Large network community detection by fast label propagation. Scientific Reports 13, 1 (2023), 2701.

[10] Leskovec Jure. 2014. SNAP Datasets: Stanford large network dataset collection. Retrieved December 2021 from [http://snap.stanford.edu/data](http://snap.stanford.edu/data) (2014).

[11] Andrew J McMinn, Yashar Moshfeghi, and Joemon M Jose. 2013. Building a large-scale corpus for evaluating event detection on twitter. In Proceedings ofthe 22nd ACM international conference on Information & Knowledge Management. ACM, 409–418.

[12] Béatrice Mazoyer, Julia Cagé, Nicolas Hervé, and Céline Hudelot. 2020. A French Corpus for Event Detection on Twitter. In Proceedings ofthe Twelfth Language Resources and Evaluation Conference. 6220–6227.


