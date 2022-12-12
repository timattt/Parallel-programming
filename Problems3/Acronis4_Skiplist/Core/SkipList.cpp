/*
 * SkipList.cpp
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#include "SkipList.h"

#include <cstdlib>
#include <limits>
#include <bits/stdc++.h>

#include "AtomicMarkableReference.h"
#include "MarkableReference.h"
#include "SkipNode.h"
#include "../RCU/RCUManager.h"

SkipList::~SkipList() {
	delete head;
	delete NIL;
};

SkipList::SkipList() {
	NIL = new SkipNode(std::numeric_limits<int>::max(), MAX_LEVELS + 1, nullptr);
	head = new SkipNode(std::numeric_limits<int>::max()-1, MAX_LEVELS + 1, NIL);
}

int SkipList::random_level() const {
	int new_level = 1;

	while (((double) rand() / (RAND_MAX)) < PROBABILITY) {
		++new_level;
	}
	return (new_level > MAX_LEVELS ? MAX_LEVELS : new_level);
}

void SkipList::print(std::ostream &os) {

	bool marked = false;
	struct SkipNode *x = head;
	while (x->key != std::numeric_limits<int>::max()) {
		if (!marked) {
			os << "Ptr: " << (long long) x % 100 << " Key: " << x->key << " Value: " << x->value << " Level: " << x->top_level << std::endl;
		} else {
			os << "Deleted Ptr: " << (long long) x % 100 << " Key: " << x->key << " Value: " << x->value << " Level: " << x->top_level << std::endl;
		}

		x = x->forward[0].get(marked);
	}
}

uint32_t SkipList::size() {
	SkipNode * x = head;
	uint32_t size = 0;
	bool marked = false;
	while (x->forward[0].get_reference()->key != std::numeric_limits<int>::max()) {
		x = x->forward[0].get(marked); // traverse to the right
		if (!marked) {
			++size;
		}
	}
	return size;
}


bool SkipList::find_with_gc(int search_key, SkipNode **preds, SkipNode **succs) {
	bool marked = false;
	bool snip = false;

	SkipNode *pred = nullptr;
	SkipNode *curr = nullptr;
	SkipNode *succ = nullptr;

RETRY:

	pred = head;
	for (int level = MAX_LEVELS; level >= 0; --level) {
		curr = pred->forward[level].get_reference();

		while (true) {

			// проверяем, не удален ли текущий товарищ
			succ = curr->forward[level].get(marked);
			while (marked) {
				// попытка физического исключения. Мы пытаемся заставить предка указывать на потомка
				snip = pred->forward[level].compare_and_swap(curr, false, succ, false);

				// если кто-то подгадил, придется начать сначала
				if (!snip) {
					goto RETRY;
				}

				// после успешного исключения у нас будет новый текущий и новый потомок
				curr = pred->forward[level].get_reference();
				succ = curr->forward[level].get(marked);
			}

			// проверяем значение ключа
			if (curr->key < search_key) {
				pred = curr;
				curr = succ;
			} else {
				break;
			}
		}

		// сохраняем пару
		if (preds != nullptr) {
			preds[level] = pred;
		}
		if (succs != nullptr) {
			succs[level] = curr;
		}
	}

	return (curr->key == search_key);
}

void SkipList::insert(int key, int val) {
	int top_level = random_level();

	SkipNode *preds[MAX_LEVELS + 1];
	SkipNode *succs[MAX_LEVELS + 1];
	SkipNode* new_node = new SkipNode(key, val, top_level);

	while (true) {
		// находим место для вставки
		bool found = find_with_gc(key, preds, succs);
		// полный путь сверху вниз до этого места лежит в массиве preds
		// в массиве succs лежат пары для элементов из preds

		if (found) {
			// если нашли - ничего не делаем
			delete new_node;
			return;
		} else {
			// заполняем башню
			for (int level = 0; level < top_level; ++level) {
				SkipNode * succ = succs[level];
				new_node->forward[level].set(succ, false);
			}

			// между ними мы вставим новый элемент
			SkipNode * pred = preds[0];
			SkipNode * succ = succs[0];

			// вставляем потомка
			new_node->forward[0].set(succ, false);

			// пытаемся заменить родителя
			// если это не получается сделать, т.е. кто-то уже успел загадить список, то
			// повторяем процедуру заново
			if (!pred->forward[0].compare_and_swap(succ, false, new_node, false)) {
				continue;
			}

			// теперь заполняем башни для предков
			for (int level = 0; level < top_level; ++level) {
				while (true) {
					pred = preds[level];
					succ = succs[level];

					// если наша информация не верна, то есть кто-то уже успел загадить список, то
					// собираем предков и потомков башни снова
					// иначе переходим к след уровню
					if (pred->forward[level].compare_and_swap(succ, false, new_node, false)) {
						break;
					}
					find_with_gc(key, preds, succs); // CAS failed for upper level, search node to update preds and succs
				}
			}

			return;
		}
	}
}

SkipNode * SkipList::remove(int key) {
	SkipNode *preds[MAX_LEVELS + 1];
	SkipNode *succs[MAX_LEVELS + 1];
	SkipNode *succ;

	bool found = find_with_gc(key, preds, succs);

	if (!found) {
		return NULL;
	} else {

		// то, что хотим удалить
		SkipNode * node_to_remove = succs[0];

		// помечаем башню удаленной кроме самого нижнего
		bool marked = false;
		for (int level = node_to_remove->top_level - 1; level >= 1; level--) {
			succ = node_to_remove->forward[level].get(marked);
			while (!marked) {
				node_to_remove->forward[level].set_marked(true);
				succ = node_to_remove->forward[level].get(marked);
			}
		}

		// помечаем нижний уровень
		marked = false;
		succ = node_to_remove->forward[0].get(marked);// потомок
		while (true) {
			// пытаемся пометить
			bool success = node_to_remove->forward[0].compare_and_swap(succ, false, succ, true);
			succ = succs[0]->forward[0].get(marked);// потенциально новый потомок
			if (success) {
				// если получилось, то теперь переходим к физическому исключению
				find_with_gc(key, preds, succs);
				// его можно и не делать - мы ведь уже пометили
				return node_to_remove;
				// иначе получается, что кто-то другой уже удалил или удалили потомка
			} else if (marked) {// важно заменить своего потомка на правильного. На того, которого еще не удалили
				return nullptr;// поэтому если он уже удален, то мы сдаемся
			}// а иначе берем нового потомка и повторяем все снова
		}
	}
}

void SkipList::dump_graph(SkipNode ** preds, SkipNode ** succs) {
	std::remove("src.txt");
	FILE * ff = fopen("src.txt", "a+");

	fprintf(ff, "digraph mygraph {\nrankdir=\"LR\";mode=\"osage\";\n");

	// vertices
	SkipNode * x = head;//head->forward[0].get(marked);
	while (1) {

		bool fa = false;
		bool fb = false;

		for (int k = 0; k < MAX_LEVELS+1; k++) {
			if (preds != NULL && x == preds[k]) {
				fa = true;
			}
			if (succs != NULL && x == succs[k]) {
				fb = true;
			}
		}

		fprintf(ff, "subgraph clusternodes%d {\n", x->key);
		if (fa && fb) {
			fprintf(ff, "color=yellow;\nstyle=filled;\n");
		} else if (fa) {
			fprintf(ff, "color=red;\nstyle=filled;\n");
		} else if (fb) {
			fprintf(ff, "color=orange;\nstyle=filled;\n");
		}
		for (int i = x->top_level-1; i >= 0; i--) {
			bool mark = false;
			x->forward[i].get(mark);
			if (mark) {
				fprintf(ff, "vert%dgg%d[label=\"%d\", color=gray, style=filled]\n", (unsigned)(long long)x, i, x->key);
			} else
			if (i == 0) {
				fprintf(ff, "vert%dgg%d[label=\"%d\", color=green, style=filled]\n", (unsigned)(long long)x, i, x->key);
			} else {
				fprintf(ff, "vert%dgg%d[label=\"%d\"]\n", (unsigned)(long long)x, i, x->key);
			}

		}
		fprintf(ff, "}\n");

		if (x == NIL) {
			break;
		}
		x = x->forward[0].get_reference();
	}

	// connections
	x = head;//head->forward[0].get_reference();
	while (x != NIL) {
		for (int i = 0; i < x->top_level && (x->forward[i].get_reference() != NIL || i == 0); i++) {
			fprintf(ff, "vert%dgg%d -> vert%dgg%d\n", (unsigned)(long long)x, i, (unsigned)(long long)x->forward[i].get_reference(), i);
		}

		x = x->forward[0].get_reference();
	}

	fprintf(ff, "}\n");

	fclose(ff);
	char cmd[100] = {0};
	sprintf(cmd, "dot -Tpng src.txt -o src.png");
	system(cmd);
}

bool SkipList::conts(SkipNode *nd) {
	SkipNode* x = head;//head->forward[0].get_reference();
	while (x != NIL) {
		if (x == nd) {
			return 1;
		}
		for (int i = 0; i < x->top_level; i++) {
			if (x->forward[i].get_reference() == nd) {
				return 1;
			}
		}

		x = x->forward[0].get_reference();
	}

	return 0;
}
