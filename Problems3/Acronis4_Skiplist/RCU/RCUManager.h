/*
 * RCUManager.h
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#ifndef RCU_RCUMANAGER_H_
#define RCU_RCUMANAGER_H_

#include <atomic>
#include <vector>

class RCUManager {
public:
	RCUManager(int threadCount);
	~RCUManager() {};

	void startReading(int threadNum);
	void endReading(int threadNum);

	void startDeleting(int threadNum);

	int threadsCount;
	std::atomic<bool> reading[100];
};

#endif /* RCU_RCUMANAGER_H_ */
