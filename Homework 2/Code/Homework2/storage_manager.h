//Name: Zongjian Li, USC ID: 6503378943
#pragma once

#include "storage.h"

#if defined(SEARCH_MODE) || defined(INTERACT_MODE)
#include <iomanip>
#endif

template <typename E>
class StorageManager {
private:
	const string FilenamePrefix;
	array<std::shared_ptr<RecordStorage<E>>, MAX_STEP + 1> Stores;

	string Filename(const Step step) {
		assert(0 <= step && step <= MAX_STEP);
		return FilenamePrefix + std::to_string(step);
	}

public:

	StorageManager(const string& _filenamePrefix) : FilenamePrefix(_filenamePrefix) {
		for (auto i = 0; i <= MAX_STEP; i++) {
			Stores[i] = std::make_shared<MemoryRecordStorage<E>>();
		}
	}

	void Serialize() {
#ifdef SEARCH_MODE
		array<std::unique_ptr<thread>, MAX_STEP + 1> threads;
		for (auto i = 0; i < MAX_STEP + 1; i++) {
			threads[i] = std::make_unique<thread>(&RecordStorage<E>::Serialize, Stores[i], Filename(i));
		}
		for (auto& t : threads) {
			t->join();
		}
#else
		for (auto i = 0; i <= MAX_STEP; i++) {
			Stores[i]->Serialize(Filename(i));
		}
#endif
	}

	void Deserialize() {
#ifdef SEARCH_MODE
		array<std::unique_ptr<thread>, MAX_STEP + 1> threads;
		for (auto i = 0; i < MAX_STEP + 1; i++) {
			threads[i] = std::make_unique<thread>(&RecordStorage<E>::Deserialize, Stores[i], Filename(i));
		}
		for (auto& t : threads) {
			t->join();
		}
#else
		for (auto i = 0; i <= MAX_STEP; i++) {
			Stores[i]->Deserialize(Filename(i));
		}
#endif
	}

	inline bool Get(const Step finishedStep, const Board board, E& record) {
		return Stores[finishedStep]->Get(board, record);
	}

	inline void Set(const Step finishedStep, const Board board, const E& record) {
		Stores[finishedStep]->Set(board, record);
	}

#if defined(SEARCH_MODE) || defined(INTERACT_MODE)
	void Report() const {
		cout << "Report:" << endl;
		for (auto i = 0; i <= MAX_STEP; i++) {
			auto& store = *Stores[i];
			string type;
			switch (store.Type()) {
			case RecordStorageType::Cache:
				type = "Cache";
				break;
			case RecordStorageType::Memory:
				type = "Memory";
				break;
			case RecordStorageType::External:
				type = "External";
				break;
			}
			cout << "\t" << i;
			cout << "\t" << store.size();
#ifdef COLLECT_STORAGE_HIT_RATE
			cout << "\t" << std::setprecision(5) << store.HitRate();
#endif
			cout << "\t" << type;
			cout << "\t" << (store.EnableSerialize ? "Serialize" : "");
			cout << endl;
		}
	}
#endif

	void Clear(const Step finishedStep) {// must stop the world
		Stores[finishedStep]->Clear();
	}

	void ClearAll() {
		for (auto i = 0; i < MAX_STEP + 1; i++) {
			Clear(i);
		}
	}

	void EnableSerialize(const Step finishedStep, const bool flag) {
		Stores[finishedStep]->EnableSerialize = flag;
	}

	bool EnableSerialize(const Step finishedStep) {
		return Stores[finishedStep]->EnableSerialize;
	}

	void SwitchBackend(const Step step, std::shared_ptr<RecordStorage<E>>&& newBackend) {// must stop the world
		assert(newBackend != nullptr);
		auto& store = Stores[step];
		cout << "Switching backend" << endl;
		auto filename = Filename(step);
		store->Serialize(filename);
		store = newBackend;
		store->Deserialize(filename);
	}

#ifdef COLLECT_STORAGE_HIT_RATE
	void ClearAllHitRate() {
		for (auto i = 0; i < MAX_STEP + 1; i++) {
			Stores[i]->ClearHitRate();
		}
	}
#endif
};