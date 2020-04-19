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


TRAIN_IMG_FILENAME = sys.argv[1] if len(sys.argv) >= 2 else "train_image.csv"
TRAIN_LBL_FILENAME = sys.argv[2] if len(sys.argv) >= 3 else "train_label.csv"
TEST_IMG_FILENAME = sys.argv[3] if len(sys.argv) >= 4 else "test_image.csv"
TEST_LBL_FILENAME = sys.argv[4] if len(sys.argv) >= 5 else "test_label.csv"
TEST_PRED_LEB_FILENAME = "test_predictions.csv"

SUBMIT = len(sys.argv) <= 4


np.random.seed(0)

#region data
def load_image(filename):
	image = np.loadtxt(filename, np.ubyte, delimiter=",")
	image = np.interp(image, (0, 255), (0, 1)).astype(np.single)
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

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def softmax(x):
	assert x.ndim == 1
	ex = np.exp(x)
	return ex / ex.sum()

def cross_entropy(picked):
	return -np.log(picked) + 0.

class Net:
	def __init__(self, size = (INPUT_SIZE, 20, 20, OUTPUT_SIZE), weight = None, bias = None):
		self.size = size
		self.weight = [np.random.randn(size[layer], size[layer - 1]).astype(np.single) for layer in range(1, len(size))] if weight is None else weight
		self.bias = [np.random.randn(size[layer]).astype(np.single) for layer in range(1, len(size))] if bias is None else bias

	def feed_forward(self, data : np.ndarray):
		assert 0.0 <= data.min() and data.max() <= 1.0
		assert data.ndim == 2 and data.shape[1] == self.size[0]
		num_samples = data.shape[0]
		neuron_out = [np.zeros((num_samples, self.size[layer]), dtype=np.single) for layer in range(1, len(self.size))]
		for layer in range(1, len(self.size)):
			layer_local = layer - 1
			# mult-add
			source = neuron_out[layer_local - 1] if layer_local != 0 else data
			multadd = source @ self.weight[layer_local].T + self.bias[layer_local]
			# sigmoid
			neuron_out[layer_local] = sigmoid(multadd)
		# softmax
		return neuron_out

	def predict(self, data : np.ndarray):
		output = self.feed_forward(data)
		return np.argmax(output[-1], axis=1)

	def back_propagation(self, loss_out, neuron_out, data, learn_rate):
		assert 0.0 <= data.min() and data.max() <= 1.0
		assert loss_out.dtype == self.weight[-1].dtype and loss_out.ndim == 2 and loss_out.shape[1] == self.size[-1]
		assert len(neuron_out) == len(self.size) - 1
		for layer in range(1, len(self.size)):
			assert neuron_out[layer - 1].shape == (loss_out.shape[0], self.size[layer])
		num_samples = data.shape[0]
		prev_loss_out_mult_d_out_net = None
		prev_old_weight = None
		for layer in range(len(self.size) - 1, 0, -1):
			layer_local = layer - 1

			if layer < len(self.size) - 1:
				w_total_shape = (num_samples, self.size[layer], self.size[layer + 1])
				d_next_loss_next_net = np.tile(prev_loss_out_mult_d_out_net, (1, self.size[layer])).reshape(w_total_shape)
				d_next_net_out = np.tile(prev_old_weight.T, (num_samples, 1)).reshape(w_total_shape)
				loss_out = (d_next_loss_next_net * d_next_net_out).sum(axis=2) # [num_samples, size[layer]]

			d_out_net = neuron_out[layer_local] * (1 - neuron_out[layer_local]) # num_samples * size[layer]

			prev_out = neuron_out[layer_local - 1] if layer_local > 0 else data
			w_total_shape = (num_samples, self.size[layer], self.size[layer - 1]) # num_samples samples, size[layer] neurons, each neuron size[layer - 1] weights
			d_net_w = np.tile(prev_out, (1, self.size[layer])).reshape(w_total_shape) # [num_sample, size[layer - 1]] => expand by repeat axis=1 size[layer] times
			
			loss_out_mult_d_out_net_raw = loss_out * d_out_net # [num_samples, size[layer]]
			loss_out_mult_d_out_net = np.repeat(loss_out_mult_d_out_net_raw, self.size[layer - 1], axis=1).reshape(w_total_shape) # [num_sample, size[layer]] => expand by repeat each elem in place size[layer - 1] times => reshape to [num_samples, size[layer], size[layer - 1]]
			d_w = loss_out_mult_d_out_net * d_net_w # element wise mult, [num_samples, size[layer], size[layer - 1]]

			prev_old_weight = self.weight[layer_local]
			prev_loss_out_mult_d_out_net = loss_out_mult_d_out_net_raw

			d_w_sum =  d_w.sum(axis=0)
			d_b_sum = loss_out.sum(axis=0)

			self.weight[layer_local] -= learn_rate * d_w_sum
			self.bias[layer_local] -= learn_rate * d_b_sum
			
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
		neuron_out = self.net.feed_forward(batch_data)

		# softmax
		softmax_out = np.apply_along_axis(softmax, -1, neuron_out[-1])

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
		self.net.back_propagation(d_softmax_sigmoid, neuron_out, batch_data, learn_rate)

		return loss.sum()

	def epoch(self, learn_rate):
		num_samples = self.train_data.shape[0]
		if self.batch_size is None or self.batch_size >= num_samples:
			return self.__train_iter(self.train_data, self.train_label, learn_rate)
		else:
			full = np.hstack((self.train_label[:, np.newaxis], self.train_data))
			np.random.shuffle(full)
			num_batch = math.ceil(num_samples / self.batch_size)
			batch_learn_rate = learn_rate #/ num_batch
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

def test_net():
	net_size = (2, 2, 2)
	weight = [
		np.array([[.15, .20], [.25, .30]]),
		np.array([[.40, .45], [.50, .55]]),
	]
	bias = [
		np.array([.35, .35]),
		np.array([.60, .60]),
	]
	net = Net(net_size, weight, bias)
	input = np.array([[.05, .10]])
	out = net.feed_forward(input)
	assert len(out) == 2 and np.allclose(out[-1], np.array([0.75136079 , 0.772928465]))
	target = np.array([0.01 , 0.99], dtype=weight[-1].dtype)
	d_loss = - (target - out[-1])
	assert np.allclose(d_loss, np.array([0.74136507, -0.217071535]))
	learn_rate = 0.5
	net.back_propagation(d_loss, out, input, learn_rate)
	new_weight = net.weight
	assert new_weight[1].dtype == weight[1].dtype and np.allclose(new_weight[1], np.array([[0.35891648, 0.408666186], [0.511301270, 0.561370121]]))
	assert new_weight[0].dtype == weight[0].dtype and np.allclose(new_weight[0], np.array([[0.149780716, 0.19956143], [0.24975114, 0.29950229]]), 0.001)
	new_bias = net.bias
	assert new_bias[1].dtype == bias[1].dtype
	assert new_bias[0].dtype == bias[0].dtype
	print("feed_forward and back_propagation check passed")

def main():
	start = time.time()
	train_img, train_lbl = load_train_pair()
	test_img, test_lbl = (load_test_image(), None) if SUBMIT else load_test_pair()
	print("data loaded, time = %3f" % (time.time() - start))
	net = Net((INPUT_SIZE, 200, 200, OUTPUT_SIZE))
	opt = Optimizer(net, train_img, train_lbl, learn_rate=0.01, decay=0.95, batch_size=32, test_data=test_img, test_label=test_lbl)
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