// This part is generally a re-implemention of the paris algorithm (https://github.com/tbonald/paris/blob/master/paris.py)

#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <RcppEigen.h>
#include <Rcpp.h>

#include "HGC_unordered_map.h"

using namespace Rcpp;
using namespace RcppEigen;

// define Inf
double const inf = R_PosInf;

// [[Rcpp::export]]
NumericMatrix HierarCluster_paris(Eigen::SparseMatrix<double> m) {
    Graph G(m);
    int n = G.adj.size();
    // cluster sizes
    IVector s(n, 1);

    // connected components
    std::vector<IVector> cc;
    // dendrogram as list of merges
    std::vector<NVector> D;

    int u = n;
    while (n > 0) {
        IVector chain;
        bool flag_single = true;
        for (int i = 0; i < G.adj.size(); i++) {
            if (!G.adj[i].empty()) {
                chain.push_back(G.adj[i].begin()->first);
                flag_single = false;
                break;
            }
        }
        if (flag_single) {
            for (int i = 0; i < G.adj.size(); i++) {
                if (G.wn[i] > 0) {
                    chain.push_back(i);
                    break;
                }
            }
        }

        while (chain.size() != 0) {
            int a = chain[chain.size() - 1];
            std::vector<int>::iterator itera = chain.begin();
            chain.erase(itera + chain.size() - 1);
            double dmin = inf;
            int b = -1;
            for (std::unordered_map<int, double>::iterator iter = G.adj[a].begin(); iter != G.adj[a].end(); iter++) {
                int v = iter->first;
                double w_va = iter->second;
                if(w_va == 0){
                    stop("There is 0 weight!");
                }
                double d = G.wn[v] * G.wn[a] / w_va / G.wtot;
                if (d < dmin) {
                    b = v;
                    dmin = d;
                }
                else if (d == dmin) {
                    b = (b <= v) ? b : v;
                }
            }

            double d = dmin;
            if (chain.size() != 0) {
                int c = chain[chain.size() - 1];
                std::vector<int>::iterator iterc = chain.begin();
                chain.erase(iterc + chain.size() - 1);
                if (b == c) {
                    // merge a, b
                    double tempa[4] = { a, b, d, s[a] + s[b] };
                    NVector tempnv(tempa, tempa + 4);
                    D.push_back(tempnv);
                    // std::cout << "acc: " << tempnv[0] << " b: " << tempnv[1] << " d: " << tempnv[2] << " scc: " << tempnv[3] << "\n";
                    // update graph
                    G.merge_node(a, b, u);
                    n--;
                    // update weight and size
                    G.wn.push_back(G.wn[a] + G.wn[b]);
                    G.wn[a] = 0;
                    G.wn[b] = 0;
                    s.push_back(s[a] + s[b]);
                    s[a] = 0;
                    s[b] = 0;
                    u++;
                }
                else {
                    chain.push_back(c);
                    chain.push_back(a);
                    chain.push_back(b);
                }
            }
            else if (b >= 0) {
                chain.push_back(a);
                chain.push_back(b);
            }
            else {
                // remove the connected component
                int tempa[2] = { a, s[a] };
                IVector tempiv(tempa, tempa + 2);
                cc.push_back(tempiv);
                G.del_node(a);
                G.wn[a] = 0;
                s[a] = 0;
                n--;
            }
        }
    }
    // std::cout << "n:  " << n << "\n";
    // std::cout << "u:  " << u << "\n";
    // std::cout << "CCsize:  " << cc.size() << "\n";
    // std::cout << "Dsize:  " << D.size() << "\n";

    if (!cc.empty()) {
        IVector cc_pop = cc[cc.size() - 1];
        int a_cc = cc_pop[0];
        int s_cc = cc_pop[1];
        cc.pop_back();
        for (int i = 0; i < cc.size(); i++) {
            IVector bt = cc[i];
            int b = bt[0];
            int t = bt[1];
            s_cc = s_cc + t;
            double tempa[4] = { a_cc, b, inf, s_cc };
            NVector tempnv(tempa, tempa + 4);
            //std::cout << "acc: " << tempnv[0] << " b: " << tempnv[1] << " d: " << tempnv[2] << " scc: " << tempnv[3] << "\n";
            D.push_back(tempnv);
            a_cc = u;
            u++;
        }
    }

    return(reorder_dendrogram(D));
}
