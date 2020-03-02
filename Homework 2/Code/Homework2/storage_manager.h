#pragma once

#include "storage.h"

class RecordManager {
private:
	const string FilenamePrefix;
	array<std::shared_ptr<RecordStorage>, MAX_STEP> Stores;

	string Filename(const Step step) {
		assert(0 <= step && step < MAX_STEP);
		return FilenamePrefix + std::to_string(step + 1);
	}

public:

	RecordManager(const string& _filenamePrefix) : FilenamePrefix(_filenamePrefix) {
		for (auto i = 0; i < MAX_STEP; i++) {
			Stores[i] = std::make_shared<MemoryRecordStorage>();

		}
	}

	void Serialize() {
		for (auto i = 0; i < MAX_STEP; i++) {
			Stores[i]->Serialize(Filename(i));
		}
	}

	void Deserialize() {
		for (auto i = 0; i < MAX_STEP; i++) {
			Stores[i]->Deserialize(Filename(i));
		}
	}

	inline bool Get(const Step finishedStep, const Board board, Record& record) {
		auto index = finishedStep - 1;
		return Stores[index]->Get(board, record);
	}

	inline void Set(const Step finishedStep, const Board board, const Record& record) {
		auto index = finishedStep - 1;
		Stores[index]->Set(board, record);
	}

	void Report() const {
		cout << "Report:" << endl;
		for (auto i = 0; i < MAX_STEP; i++) {
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
			cout << "\t" << i + 1;
			cout << "\t" << store.size();
#ifdef COLLECT_STORAGE_HIT_RATE
			cout << "\t" << std::setprecision(5) << store.HitRate();
#endif
			cout << "\t" << type;
			cout << "\t" << (store.EnableSerialize ? "Serialize" : "");
			cout << endl;
		}
	}

	void Clear(const Step step) {// must stop the world
		auto index = step - 1;
		Stores[index]->Clear();
	}

	void EnableSerialize(const Step step, const bool flag) {
		auto index = step - 1;
		Stores[index]->EnableSerialize = flag;
	}

	bool EnableSerialize(const Step step) {
		auto index = step - 1;
		return Stores[index]->EnableSerialize;
	}

	void SwitchBackend(const Step step, std::shared_ptr<RecordStorage>&& newBackend) {// must stop the world
		assert(newBackend != nullptr);
		auto index = step - 1;
		auto& store = Stores[index];
		cout << "Switching backend" << endl;
		auto filename = Filename(index);
		store->Serialize(filename);
		store = newBackend;
		store->Deserialize(filename);
	}
};