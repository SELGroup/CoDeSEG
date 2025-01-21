import re
from itertools import combinations

import numpy as np
import pandas as pd
from os.path import exists
import pickle

from tqdm import tqdm


from sentence_transformers import SentenceTransformer

def replaceAtUser(text):
    """ Replaces "@user" with "" """
    text = re.sub('@[^\s]+|RT @[^\s]+','',text)
    return text

def removeUnicode(text):
    """ Removes unicode strings like "\u002c" and "x96" """
    text = re.sub(r'(\\u[0-9A-Fa-f]+)',r'', text)
    text = re.sub(r'[^\x00-\x7f]',r'',text)
    return text

def replaceURL(text):
    """ Replaces url address with "url" """
    text = re.sub('((www\.[^\s]+)|(https?://[^\s]+))','url',text)
    text = re.sub(r'#([^\s]+)', r'\1', text)
    return text

def replaceMultiExclamationMark(text):
    """ Replaces repetitions of exlamation marks """
    text = re.sub(r"(\!)\1+", '!', text)
    return text

def replaceMultiQuestionMark(text):
    """ Replaces repetitions of question marks """
    text = re.sub(r"(\?)\1+", '?', text)
    return text

def removeEmoticons(text):
    """ Removes emoticons from text """
    text = re.sub(':\)|;\)|:-\)|\(-:|:-D|=D|:P|xD|X-p|\^\^|:-*|\^\.\^|\^\-\^|\^\_\^|\,-\)|\)-:|:\'\(|:\(|:-\(|:\S|T\.T|\.\_\.|:<|:-\S|:-<|\*\-\*|:O|=O|=\-O|O\.o|XO|O\_O|:-\@|=/|:/|X\-\(|>\.<|>=\(|D:', '', text)
    return text

def removeNewLines(text):
    text = re.sub('\n', '', text)
    return text

def preprocess_sentence(s):
    return removeNewLines(replaceAtUser(removeEmoticons(replaceMultiQuestionMark(replaceMultiExclamationMark(removeUnicode(replaceURL(s)))))))
def SBERT_embed(s_list, language = 'English'):
    '''
    Use Sentence-BERT to embed sentences.
    s_list: a list of sentences/ tokens to be embedded.
    output: the embeddings of the sentences/ tokens.
    '''

    if language == 'English':
        model = SentenceTransformer('../PLMS/SBERT/') # for English
    elif language == 'French':
        model = SentenceTransformer('../PLMS/SBERTFR/') # for French
    embeddings = model.encode(s_list, convert_to_tensor = True, normalize_embeddings = True)

    # device = torch.device("cuda:1" if torch.cuda.is_available() else "cpu")
    # model.to(device)
    # data_loader = DataLoader(s_list, batch_size=1000)
    # embeddings = []
    # with torch.no_grad():
    #     for batch in data_loader:
    #         input_ids = tokenizer(batch, return_tensors="pt", padding=True)
    #         input_ids.to(device)
    #         outputs = model(**input_ids)
    #         embeddings.append(outputs.last_hidden_state[:, 0, :].cpu())
    #     embeddings = torch.cat(embeddings, dim=0)
    # embeddings = F.normalize(embeddings, p=2, dim=1)
    return embeddings.cpu()
def get_embedding(df, name):
    data_path = f'../graph_dataset/{name}/'
    if not exists(data_path + 'embeddings.pkl'):
        processed_text = [preprocess_sentence(s) for s in
                          df['text'].values]  # hastags are kept (with '#' removed). RTs are removed.
        if name == "tweet2012":
            embeddings = SBERT_embed(processed_text, language='English')
        else:
            embeddings = SBERT_embed(processed_text, language='French')
        with open(data_path + 'embeddings.pkl', 'wb') as fp:
            pickle.dump(embeddings, fp)
        print('SBERT embeddings stored.')

def get_att_edges(df,name):

    attributes = [[str(u)] + \
                  [str(each) for each in um] + \
                  [h.lower() for h in hs]
                  for u, um, hs,  in \
                  zip(df['user_id'], df['user_mentions'], df['hashtags'])]
    attr_nodes_dict = {}
    for i, l in enumerate(attributes):
        for attr in l:
            if attr not in attr_nodes_dict:
                attr_nodes_dict[attr] = [i]
            else:
                attr_nodes_dict[attr].append(i)

    for attr in attr_nodes_dict.keys():
        attr_nodes_dict[attr].sort()

    graph_edges = []
    data_path = f'../graph_dataset/{name}/'
    for l in attr_nodes_dict.values():
        graph_edges += list(combinations(l, 2))


    with open(data_path+'embed.pkl', 'rb') as f:
        embeddings = pickle.load(f)

    corr_matrix = np.corrcoef(embeddings)
    np.fill_diagonal(corr_matrix, 0)

    edges_np = np.array(graph_edges)
    edges_sorted = np.sort(edges_np, axis=1)
    edges = np.unique(edges_sorted, axis=0)
    w_edges = set()

    bar = tqdm(total=edges.shape[0], desc="Adding attribute edge weight")
    t, k = 0.2, 150
    for a, b in edges:
        w = corr_matrix[a, b]
        if w > t:
            w_edges.add((a, b, w))
        bar.update(1)
    bar.close()
    bar = tqdm(total=corr_matrix.shape[0], desc="Adding Semantic edge")
    for a, row in enumerate(corr_matrix):
        sorted_indices = np.argsort(row)
        top_10_indices = sorted_indices[-k:]
        for ind in top_10_indices:
            w = corr_matrix[a, ind]
            if w > 0:
                w_edges.add((a, ind, corr_matrix[a, ind]) if a < ind else (ind, a, corr_matrix[ind,a]))
        bar.update(1)
    bar.close()

    f_edges = list(w_edges)
    with open(data_path + "awgraph.txt", 'w', encoding='utf8') as output_file:
        bar = tqdm(total=len(f_edges), desc="writing")
        for a, b, w in f_edges:
            edge = f"{str(a)}\t{str(b)}\t{str(w)}"
            output_file.write(edge + '\n')
            bar.update(1)
        bar.close()
    np.save(data_path + f'awedges.npy', f_edges)


def preprocess(name):
    if name == "tweet2012":
        df_np = np.load('../graph_dataset/tweet_2012/All_English.npy', allow_pickle=True)
        df = pd.DataFrame(data=df_np, columns=["event_id", "tweet_id", "text", "user_id", "created_at", "user_loc", \
                                               "place_type", "place_full_name", "place_country_code", "hashtags",
                                               "user_mentions", "image_urls", "entities",
                                               "words", "filtered_words", "sampled_words"])
    else:
        df_np = np.load('../graph_dataset/tweet_2018/All_French.npy', allow_pickle=True)
        df = pd.DataFrame(data=df_np, columns=["tweet_id", "user_id", "text", "time", "event_id", "user_mentions", \
                                               "hashtags", "urls", "words", "created_at", "filtered_words", "entities",
                                               "sampled_words"])

    data_path = f'../graph_dataset/{name}/'
    labels = df["event_id"].values
    np.save(data_path + 'labels.npy', labels)

    cmty={}
    for node, comm in enumerate(labels):
        if comm not in cmty:
            cmty[comm] = []
        cmty[comm].append(node)
    with open(data_path + "cmty.txt", 'w', encoding='utf8') as output_file:
        for community, nodes in cmty.items():
            output_file.write("\t".join([str(n) for n in nodes]))
            output_file.write("\n")

    get_embedding(ini_df, name)

    get_att_edges(df,name)



if __name__ == "__main__":
    with open("../graph_dataset/tweet_2012/cmty.txt", 'r') as nse_file:
        lines = nse_file.readlines()
    for name in [ "tweet2012", "tweet2018"]:
        preprocess(name)
