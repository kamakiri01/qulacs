#pragma once

#include "gate.hpp"
#include "state.hpp"
#include "utility.hpp"

/**
 * \~japanese-en 確率的なユニタリ操作
 */
class QuantumGate_Probabilistic : public QuantumGateBase {
protected:
	Random random;
	std::vector<double> _distribution;
	std::vector<double> _cumulative_distribution;
	std::vector<QuantumGateBase*> _gate_list;

public:
	/**
	 * \~japanese-en コンストラクタ
	 *
	 * @param distribution ゲートが現れる確率
	 * @param gate_list ゲートのリスト
	 */
	QuantumGate_Probabilistic(std::vector<double> distribution, std::vector<QuantumGateBase*> gate_list) {
		_distribution = distribution;

		double sum = 0.;
		_cumulative_distribution.push_back(0.);
		for (auto val : distribution) {
			sum += val;
			_cumulative_distribution.push_back(sum);
		}
		_gate_list = gate_list;
	};

	/**
	 * \~japanese-en 量子状態を更新する
	 *
	 * @param state 更新する量子状態
	 */
	virtual void update_quantum_state(QuantumStateBase* state) override {

		double r = random.uniform();
		auto ite = std::lower_bound(_cumulative_distribution.begin(), _cumulative_distribution.end(), r);
		assert(ite != _cumulative_distribution.begin());
		size_t gate_index = std::distance(_cumulative_distribution.begin(), ite) - 1;

		if (gate_index < _gate_list.size()) {
			_gate_list[gate_index]->update_quantum_state(state);
		}
	};
	/**
	 * \~japanese-en 自身のディープコピーを生成する
	 *
	 * @return 自身のディープコピー
	 */
	virtual QuantumGateBase* copy() const override {
		std::vector<QuantumGateBase*> new_gate_list;
		for (auto item : _gate_list) {
			new_gate_list.push_back(item->copy());
		}
		return new QuantumGate_Probabilistic(_distribution, new_gate_list);
	};

	/**
	 * \~japanese-en 自身のゲート行列をセットする
	 *
	 * @param matrix 行列をセットする変数の参照
	 */
	virtual void set_matrix(ComplexMatrix& matrix) const override {
		std::cerr << "* Warning : Gate-matrix of probabilistic gate cannot be obtained. Identity matrix is returned." << std::endl;
		matrix = Eigen::MatrixXcd::Ones(1, 1);
	}
};

/**
 * \~japanese-en Kraus表現のCPTP-map
 */
class QuantumGate_CPTP : public QuantumGateBase {
protected:
	Random random;
	std::vector<QuantumGateBase*> _gate_list;

public:
	QuantumGate_CPTP(std::vector<QuantumGateBase*> gate_list) {
		_gate_list = gate_list;
	};

	/**
	 * \~japanese-en 量子状態を更新する
	 *
	 * @param state 更新する量子状態
	 */
	virtual void update_quantum_state(QuantumStateBase* state) override {
		double r = random.uniform();

		double sum = 0.;
		QuantumGateBase* chosen_gate = NULL;
		double org_norm = state->get_norm();

		auto buffer = state->copy();
		double norm;
		for (auto gate : _gate_list) {
			gate->update_quantum_state(buffer);
			norm = buffer->get_norm()/org_norm;
			sum += norm;
			if (r < sum) {
				state->load(buffer);
				state->normalize(norm);
				break;
			}
			else {
				buffer->load(state);
			}
		}
		if (!(r < sum)) {
			std::cerr << "* Warning : CPTP-map was not trace preserving. Identity-map is applied." << std::endl;
		}
		delete buffer;
	};

	/**
	 * \~japanese-en 自身のディープコピーを生成する
	 *
	 * @return 自身のディープコピー
	 */
	virtual QuantumGateBase* copy() const override {
		std::vector<QuantumGateBase*> new_gate_list;
		for (auto item : _gate_list) {
			new_gate_list.push_back(item->copy());
		}
		return new QuantumGate_CPTP(new_gate_list);
	};
	/**
	 * \~japanese-en 自身のゲート行列をセットする
	 *
	 * @param matrix 行列をセットする変数の参照
	 */
	virtual void set_matrix(ComplexMatrix& matrix) const override {
		std::cerr << "* Warning : Gate-matrix of CPTP-map cannot be obtained. Identity matrix is returned." << std::endl;
		matrix = Eigen::MatrixXcd::Ones(1, 1);
	}
};

/**
 * \~japanese-en Instrument
 */
class QuantumGate_Instrument : public QuantumGateBase {
protected:
	Random random;
	std::vector<QuantumGateBase*> _gate_list;
	UINT _classical_register_address;

public:
	QuantumGate_Instrument(std::vector<QuantumGateBase*> gate_list, UINT classical_register_address) {
		_classical_register_address = classical_register_address;
		_gate_list = gate_list;
	};

	/**
	 * \~japanese-en 量子状態を更新する
	 *
	 * @param state 更新する量子状態
	 */
	virtual void update_quantum_state(QuantumStateBase* state) override {
		double r = random.uniform();

		double sum = 0.;
		QuantumGateBase* chosen_gate = NULL;
		double org_norm = state->get_norm();

		auto buffer = state->copy();
		double norm;
		UINT index = 0;
		for (auto gate : _gate_list) {
			gate->update_quantum_state(buffer);
			norm = buffer->get_norm() / org_norm;
			sum += norm;
			if (r < sum) {
				state->load(buffer);
				state->normalize(norm);
				break;
			}
			else {
				buffer->load(state);
				index++;
			}
		}
		if (!(r < sum)) {
			std::cerr << "* Warning : Instrument-map was not trace preserving. Identity-map is applied." << std::endl;
		}
		delete buffer;

		state->set_classical_value(_classical_register_address, index);
	};
	/**
	 * \~japanese-en 自身のディープコピーを生成する
	 *
	 * @return 自身のディープコピー
	 */
	virtual QuantumGateBase* copy() const override {
		std::vector<QuantumGateBase*> new_gate_list;
		for (auto item : _gate_list) {
			new_gate_list.push_back(item->copy());
		}
		return new QuantumGate_Instrument(new_gate_list, _classical_register_address);
	};
	/**
	 * \~japanese-en 自身のゲート行列をセットする
	 *
	 * @param matrix 行列をセットする変数の参照
	 */
	virtual void set_matrix(ComplexMatrix& matrix) const override {
		std::cerr << "* Warning : Gate-matrix of Instrument cannot be obtained. Identity matrix is returned." << std::endl;
		matrix = Eigen::MatrixXcd::Ones(1, 1);
	}
};


/**
 * \~japanese-en Adaptiveな操作
 */
class QuantumGate_Adaptive : public QuantumGateBase {
protected:
	QuantumGateBase* _gate;
	std::function<bool(const std::vector<UINT>&)> _func;
public:
	QuantumGate_Adaptive(QuantumGateBase* gate, std::function<bool(const std::vector<UINT>&)> func) {
		_gate = gate;
		_func = func;
	};

	/**
	 * \~japanese-en 量子状態を更新する
	 *
	 * @param state 更新する量子状態
	 */
	virtual void update_quantum_state(QuantumStateBase* state) override {
		bool result = _func(state->get_classical_register());
		if (result) {
			_gate->update_quantum_state(state);
		}
	};
	/**
	 * \~japanese-en 自身のディープコピーを生成する
	 *
	 * @return 自身のディープコピー
	 */
	virtual QuantumGateBase* copy() const override {
		return new QuantumGate_Adaptive(_gate->copy(), _func);
	};
	/**
	 * \~japanese-en 自身のゲート行列をセットする
	 *
	 * @param matrix 行列をセットする変数の参照
	 */
	virtual void set_matrix(ComplexMatrix& matrix) const override {
		std::cerr << "* Warning : Gate-matrix of Adaptive-gate cannot be obtained. Identity matrix is returned." << std::endl;
		matrix = Eigen::MatrixXcd::Ones(1, 1);
	}
};

