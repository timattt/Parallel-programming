/*
 * RCUManager.cpp
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#include "RCUManager.h"
#include <chrono>
#include <cstdint>
#include <bits/stdc++.h>

RCUManager::RCUManager(int threadCount) : threadsCount(threadCount) {
	for (int i = 0; i < threadCount; i++) {
		reading[i] = false;
	}
}

void RCUManager::startReading(int threadNum) {
	reading[threadNum].store(true);
}

void RCUManager::endReading(int threadNum) {
	reading[threadNum].store(false);
}

void RCUManager::startDeleting(int threadNum) {
	std::vector<int> toWait;

	for (int i = 0; i < threadsCount; i++) {
		if (reading[i].load()) {
			toWait.push_back(i);
		}
	}

	while (!toWait.empty()) {
		for (unsigned i = 0; i < toWait.size(); i++) {
			if (reading[toWait[i]].load() == false) {
				toWait.erase(toWait.begin() + i);
				break;
			}
		}
	}
}
