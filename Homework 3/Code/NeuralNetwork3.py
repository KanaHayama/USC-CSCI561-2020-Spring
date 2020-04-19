import numpy as np
import time
import sys

IMG_HEIGHT = 28
IMG_WIDTH = 28
INPUT_SIZE = IMG_HEIGHT * IMG_WIDTH
OUTPUT_SIZE = 10
TIME_LIMIT = 30 * 60
TRAIN_TIME_LIMIT = TIME_LIMIT * 0.8
MAX_EPOCHES = 100
DATA_TYPE = np.double


TRAIN_IMG_FILENAME = sys.argv[1] if len(sys.argv) >= 2 else "train_image.csv"
TRAIN_LBL_FILENAME = sys.argv[2] if len(sys.argv) >= 3 else "train_label.csv"
TEST_IMG_FILENAME = sys.argv[3] if len(sys.argv) >= 4 else "test_image.csv"
TEST_LBL_FILENAME = sys.argv[4] if len(sys.argv) >= 5 else "test_label.csv"
TEST_PRED_LEB_FILENAME = "test_predictions.csv"

SUBMIT = len(sys.argv) <= 4

#np.seterr(all='raise')
np.random.seed(0)

#region data
def load_image(filename):
	image = np.loadtxt(filename, np.ubyte, delimiter=",")
	image = np.interp(image, (0, 255), (0, 1)).astype(DATA_TYPE)
	return image

def load_pair(img_filename, label_filename) -> (np.ndarray, np.ndarray):
	label = np.loadtxt(label_filename, np.ubyte, delimiter=",")
	image = load_image(img_filename)
	assert label.shape[0] == image.shape[0]
	return image, label

def load_train_pair() -> (np.ndarray, np.ndarray):
	return load_pair(TRAIN_IMG_FILENAME, TRAIN_LBL_FILENAME)

def load_test_pair() -> (np.ndarray, np.ndarray):
	return load_pair(TEST_IMG_FILENAME, TEST_LBL_FILENAME)

def load_test_image():
	return load_image(TEST_IMG_FILENAME)

def write_test_pred_label(pred):
	np.savetxt(TEST_PRED_LEB_FILENAME, pred, delimiter=",", fmt="%d")
#endregion
#region net
import math

def has_nan(x):
	return np.isnan(np.sum(x))

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def sigmoid_derivative(x):
	return x * (1 - x)

def relu(x):
	return np.maximum(0, x)

def relu_derivative(x):
	return np.greater(x, 0).astype(np.single)

activation = relu
activation_derivative = relu_derivative

def softmax(x):
	assert x.ndim == 2
	m = np.max(x ,axis=1).reshape(-1, 1)
	n = x - m
	ex = np.exp(n)
	return ex / ex.sum(axis=1).reshape(-1, 1)

def cross_entropy(picked):
	return -np.log(np.maximum(picked, 1e-9)) + 0.

class Net:
	def __init__(self, size = (INPUT_SIZE, 20, 20, OUTPUT_SIZE), weight = None, bias = None):
		self.size = size
		self.weight = [(np.random.randn(size[layer], size[layer - 1]) * math.sqrt( 2 / (size[layer] + size[layer - 1]))).astype(DATA_TYPE) for layer in range(1, len(size))] if weight is None else weight
		self.bias = [np.zeros(size[layer], dtype=DATA_TYPE) for layer in range(1, len(size))] if bias is None else bias

	def feed_forward(self, data : np.ndarray):
		assert 0.0 <= data.min() and data.max() <= 1.0
		assert data.ndim == 2 and data.shape[1] == self.size[0]
		num_samples = data.shape[0]
		multadd_out = [np.zeros((num_samples, self.size[layer]), dtype=np.single) for layer in range(1, len(self.size))]
		layer_out = [np.zeros((num_samples, self.size[layer]), dtype=np.single) for layer in range(1, len(self.size))]
		for layer in range(1, len(self.size)):
			layer_local = layer - 1
			# mult-add
			source = layer_out[layer_local - 1] if layer_local != 0 else data
			multadd_out[layer_local] = source @ self.weight[layer_local].T + self.bias[layer_local]
			# activation
			layer_out[layer_local] = activation(multadd_out[layer_local]) if layer < len(self.size) - 1 else multadd_out[layer_local] # do not use Relu in output layer
		return multadd_out, layer_out

	def predict(self, data : np.ndarray):
		_, layer_out = self.feed_forward(data)
		return np.argmax(layer_out[-1], axis=1)

	def back_propagation(self, loss, layer_out, multadd_out, data, learn_rate):
		num_samples = loss.shape[0]
		for layer in range(len(self.size) - 1, 0, -1):
			layer_local = layer - 1
			prev_layer_out = layer_out[layer_local - 1] if layer_local > 0 else data
			if layer < len(self.size) - 1:
				loss = np.matmul(loss, self.weight[layer_local + 1])
				d_activation = activation_derivative(multadd_out[layer_local])
				loss = np.multiply(loss, d_activation)
			delta_b = loss
			delta_w = loss[:, :, np.newaxis] * prev_layer_out[:, np.newaxis, :]
			self.weight[layer_local] -= learn_rate * delta_w.sum(axis=0)
			self.bias[layer_local] -= learn_rate * delta_b.sum(axis=0)
			
	def duplicate(self):
		return Net(self.size, self.weight, self.bias)

#endregion
#region
class Optimizer:
	def __init__(self, net, train_data, train_label, learn_rate = 0.5, decay = 0.95, batch_size = None, test_data = None, test_label = None):
		self.net = net
		self.learn_rate = learn_rate
		self.decay = decay
		self.batch_size = batch_size
		self.train_data = train_data
		self.train_label = train_label
		self.test_data = test_data
		self.test_label = test_label
		self.total_epoches = 0


	def __train_iter(self, batch_data, batch_label, learn_rate):
		# forward
		multadd_out, layer_out = self.net.feed_forward(batch_data)
		last_layer_out = layer_out[-1]

		# softmax
		softmax_out = softmax(last_layer_out)

		# loss
		pick_indices = (np.arange(batch_label.shape[0]), batch_label)
		picked = softmax_out[pick_indices]
		loss = cross_entropy(picked)

		# derivative of loss till softmax
		#	loss = - ln s_lbl => loss_d = -1 / s_lbl
		#	for i in outputs
		#		when i == lbl => s_i_d / o_i_d = s_i * (1 - s_i)
		#		when i != lbl => s_i_d / o_i_d = - s_i * s_lbl
		#	loss_d * (s_i_d / o_i_d) = i != lbl ? s_i : s_i - 1
		d_softmax_sigmoid = softmax_out # assign s_i	# NOTE this is copy by reference
		d_softmax_sigmoid[pick_indices] -= 1

		# bp
		self.net.back_propagation(d_softmax_sigmoid, layer_out, multadd_out, batch_data, learn_rate)

		return loss.sum()

	def epoch(self, learn_rate):
		num_samples = self.train_data.shape[0]
		if self.batch_size is None or self.batch_size >= num_samples:
			return self.__train_iter(self.train_data, self.train_label, learn_rate)
		else:
			full = np.hstack((self.train_label[:, np.newaxis], self.train_data))
			np.random.shuffle(full)
			num_batch = math.ceil(num_samples / self.batch_size)
			batch_learn_rate = learn_rate / num_batch
			batches = np.array_split(full, num_batch)
			total_loss = 0.0
			for batch in batches:
				batch_label, batch_data = np.hsplit(batch, (1,))
				batch_label = batch_label[:, 0].astype(np.ubyte)
				total_loss += self.__train_iter(batch_data, batch_label, batch_learn_rate)
			return total_loss

	def train(self, termination_test_func):
		while True:
			start = time.time()
			self.total_epoches += 1
			learn_rate = self.__learn_rate()
			loss = self.epoch(learn_rate)
			end = time.time()
			if self.test_data is not None and self.test_label is not None:
				_, train_accuracy = self.test(self.train_data, self.train_label)
				_, test_accuracy= self.test(self.test_data, self.test_label)
				print("epoch = %d, time = %6f, lr = %f, loss = %f, train_accuracy = %f, test_accuracy = %f" % (self.total_epoches, end - start, learn_rate, loss, train_accuracy, test_accuracy))
			else:
				print("epoch = %d, time = %6f, lr = %f, loss = %f" % (self.total_epoches, end - start, learn_rate, loss))
			if termination_test_func(self.total_epoches):
				break

	def __learn_rate(self):
		return self.learn_rate * (self.decay ** self.total_epoches)

	def __test_iter(self, batch_data, batch_label):
		num_samples = batch_data.shape[0]
		pred = self.net.predict(batch_data)
		num_correct = (pred == batch_label).sum()
		return num_correct, num_samples, pred

	def test(self, data, label):
		correct = 0
		num_samples = data.shape[0]
		pred = []
		if self.batch_size is None or self.batch_size >= num_samples:
			correct, _, pred = self.__test_iter(data, label)
		else:
			num_batch = math.ceil(num_samples / self.batch_size)
			data_batches = np.array_split(data, num_batch)
			label_batches = np.array_split(label, num_batch)
			for batch_label, batch_data in zip(label_batches, data_batches):
				batch_num_correct, _, batch_pred = self.__test_iter(batch_data, batch_label)
				correct += batch_num_correct
				pred.append(batch_pred)
			pred = np.concatenate(pred)
		accuracy = correct / num_samples
		return pred, accuracy
		
#endregion

def main():
	start = time.time()
	train_img, train_lbl = load_train_pair()
	test_img, test_lbl = (load_test_image(), None) if SUBMIT else load_test_pair()
	print("data loaded, time = %3f" % (time.time() - start))
	net = Net((INPUT_SIZE, 500, 300, OUTPUT_SIZE))
	opt = Optimizer(net, train_img, train_lbl, learn_rate=0.01, decay=0.95, batch_size=1000, test_data=test_img, test_label=test_lbl)
	term_func = lambda num_epoches: time.time() - start >= TRAIN_TIME_LIMIT or num_epoches >= MAX_EPOCHES
	opt.train(term_func)
	if SUBMIT:
		pred = opt.net.predict(test_img)
		write_test_pred_label(pred)
	else:
		_, accuracy = opt.test(test_img, test_lbl)
		print("final accuracy = %f" % accuracy)
	print("total time = %3f" % (time.time() - start))

if __name__ == "__main__":
	main()
