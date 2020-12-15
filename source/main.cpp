#include <stdio.h>
#include <map>
#include <set>
#include <vector>
#include <string.h>
#include <math.h>
#include <queue>

#include"pagerank.h"

static double now_value[BLOCK_SIZE];
std::map<int, int> id2loc;
std::set<int> node_set;
std::vector<int> id_list;

FILE* matrix, * old;

int main(int argc, char** argv) {
	// open temp files to save matrix and rank value
	matrix = tmpfile();
	old = tmpfile();
	if (matrix == NULL || old == NULL) {
		printf("temp file cannot open \n");
		return 1;
	}

	if (split_data(argc, argv))
		return 1;

	if (solve())
		return 1;

	if (output())
		return 1;

	// close files
	fclose(matrix);
	fclose(old);
	return 0;
}

int split_data(int argc, char** argv) {
	// open dataset
	FILE* fp;
	if (argc == 1)
		fp = fopen("../dataset/WikiData.txt", "r");
	else
		fp = fopen(argv[1], "r");
	//verified
	if (fp == NULL) {
		printf("dataset file cannot open \n");
		return 1;
	}

	// scan all the records
	int src, dest, t = 0;
	id_list.clear();
	node_set.clear();
	id2loc.clear();
	while (fscanf(fp, "%d%d", &src, &dest) != EOF) {
		if (node_set.count(src) == 0) {
			id_list.push_back(src);
			node_set.insert(src);
			id2loc[src] = 0;
		}
		id2loc[src] += 1;
		if (node_set.count(dest) == 0) {
			id_list.push_back(dest);
			node_set.insert(dest);
		}
		++t;
	}
	printf("Read file successfully. Has %lu websites and %d records\n", id_list.size(), t);

	// construct old rank value and matrix
	double init = 1.0 / id_list.size();
	node_set.clear();
	for (int i = 1; i <= id_list.size(); ++i) {
		node_set.insert(id_list[i - 1]);
		fwrite(&init, sizeof(double), 1, old);
		if (i % BLOCK_SIZE == 0) {
			// new block
			fseek(fp, 0, SEEK_SET);
			int pointer = 0;
			while (fscanf(fp, "%d%d", &src, &dest) != EOF) {
				if (node_set.count(dest)) {
					// the record is in the block
					if (pointer == 0) {
						fwrite((char*)&src, sizeof(int), 1, matrix);			// record start with src
						fwrite((char*)&id2loc[src], sizeof(int), 1, matrix);	// degree
					}
					else if (pointer != src) {
						// new src
						fwrite((char*)&END, sizeof(int), 1, matrix);	// record end
						fwrite((char*)&src, sizeof(int), 1, matrix);	// record start with src
						fwrite((char*)&id2loc[src], sizeof(int), 1, matrix);	// degree
					}
					fwrite((char*)&dest, sizeof(int), 1, matrix);
					pointer = src;
				}
			}
			if (pointer != 0)
				fwrite((char*)&END, sizeof(int), 1, matrix);	// record end
			fwrite((char*)&END, sizeof(int), 1, matrix);	// block end
			node_set.clear();
			//printf("block i=%d writed!\n", i / BLOCK_SIZE);
		}
	}

	// remained block
	fseek(fp, 0, SEEK_SET);
	int pointer = 0;
	while (fscanf(fp, "%d%d", &src, &dest) != EOF) {
		if (node_set.count(dest)) {
			// the record is in the block
			if (pointer == 0) {
				fwrite((char*)&src, sizeof(int), 1, matrix);			// record start with src
				fwrite((char*)&id2loc[src], sizeof(int), 1, matrix);	// degree
			}
			else if (pointer != src) {
				// new src
				fwrite((char*)&END, sizeof(int), 1, matrix);	// record end
				fwrite((char*)&src, sizeof(int), 1, matrix);	// record start with src
				fwrite((char*)&id2loc[src], sizeof(int), 1, matrix);	// degree
			}
			fwrite((char*)&dest, sizeof(int), 1, matrix);
			pointer = src;
		}
	}
	fwrite((char*)&END, sizeof(int), 1, matrix);	// record end
	fwrite((char*)&END, sizeof(int), 1, matrix);	// block end
	node_set.clear();

	// close/flush files
	fflush(old);
	fflush(matrix);
	fclose(fp);
	return 0;
}

int solve() {
	FILE* _new = tmpfile();
	// construct a map
	for (int i = 0; i < id_list.size(); ++i) {
		id2loc[id_list[i]] = i;
	}

	// the number of websites
	int web_num = id_list.size();

	/* iteration start */
	while (true) {
		// initial
		double total = .0;
		fseek(old, 0, SEEK_SET);
		fseek(_new, 0, SEEK_SET);
		fseek(matrix, 0, SEEK_SET);

		for (int i = 0; i < web_num; i += BLOCK_SIZE) {
			// initial new value in blocks
			memset(now_value, 0, sizeof(now_value));

			// for each block
			while (true) {
				int src, degree;
				fread(&src, sizeof(int), 1, matrix);
				if (src == -1)
					// block end
					break;
				// obtain the degree and the old rank value of src
				fread(&degree, sizeof(int), 1, matrix);
				double value;
				fseek(old, id2loc[src] * sizeof(double), SEEK_SET);
				fread(&value, sizeof(double), 1, old);

				// for each record
				while (true) {
					int dest;
					fread(&dest, sizeof(int), 1, matrix);
					if (dest == -1)
						// record end
						break;

					// calculate new value
					now_value[id2loc[dest] - i] += BETA * value / degree;
				}
			}

			// sum and write back
			for (int j = 0; j < BLOCK_SIZE; j++) {
				total += now_value[j];
			}
			fwrite(now_value, sizeof(double), BLOCK_SIZE, _new);
			fflush(_new);
		}

		// normalize the r_new and write them into old.bin
		total = (1.0 - total) / web_num;
		double offset = 0;
		fseek(old, 0, SEEK_SET);
		fseek(_new, 0, SEEK_SET);
		memset(now_value, 0, sizeof(now_value));
		for (int i = 0; i < web_num; i += BLOCK_SIZE / 2) {
			// read new value and old value
			fread(now_value, sizeof(double), BLOCK_SIZE / 2, _new);
			fread(now_value + BLOCK_SIZE / 2, sizeof(double), BLOCK_SIZE / 2, old);
			for (int j = 0; j < BLOCK_SIZE / 2; ++j) {
				/* normalize */
				now_value[j] += total;
				/* calculate the difference of r_old and r_new */
				offset += fabs(now_value[j] - now_value[j + BLOCK_SIZE / 2]);
			}

			fseek(old, sizeof(double) * i, SEEK_SET);
			/* write them into old.bin */
			fwrite(now_value, sizeof(double), BLOCK_SIZE / 2, old);
			fflush(old);
		}
		printf("end one iteration with %.10f bias from last one\n", offset);

		// whether convergence */
		if (offset < EPSILON) {
			break;
		}
	}
	fclose(_new);
	return 0;
}

struct cmp {
	bool operator()(std::pair<int, double> a, std::pair<int, double> b) {
		return a.second < b.second;
	}
};
std::priority_queue<std::pair<int, double>, std::vector<std::pair<int, double> >, cmp> p;

int output() {
	FILE* result = fopen("result.txt", "w");
	fseek(old, 0, SEEK_SET);
	double value;
	// use priority_queue to fine top x website
	for (int i = 0; i < id_list.size(); ++i) {
		fread(&value, sizeof(double), 1, old);
		p.push(std::make_pair(id_list[i], value));
	}

	// output results
	for (int i = 0; i < TOP && i < id_list.size(); ++i) {
		std::pair<int, double> tmp = p.top();
		int web_id = tmp.first;
		// output the result to result.txt
		fprintf(result, "%d\t%.10f\n", web_id, tmp.second);
		p.pop();
	}
	fclose(result);
	return 0;
}
