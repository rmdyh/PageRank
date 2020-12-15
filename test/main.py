import os
import networkx as nx

G=nx.DiGraph()
t=0
for line in open(os.path.join('dataset','WikiData.txt'),'r'):
    itemline = line.strip()

    item = line.split('\t')
    if len(item) < 2:
        item = line.split('   ')

    G.add_edge(int(item[0]), int(item[1]))
    t += 1

print('website number:{}, record number: {}'.format(G.number_of_nodes(),G.number_of_edges()))

import matplotlib.pyplot as plt
#nx.draw_shell(G, node_size=10,arrowsize=10,alpha=0.5, width = 0.3)
plt.show()

#exit()

pr = nx.pagerank(G, alpha=0.85, max_iter=10000, tol=1e-12)
l = [(x,pr[x]) for x in pr.keys()]
for i in sorted(l,key=lambda x:x[1],reverse=True)[:100]:
    print(i)