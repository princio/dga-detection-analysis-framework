import tensorflow as tf
from keras.utils import pad_sequences
from tensorflow import convert_to_tensor
from keras.models import load_model as keras_load_model, model_from_json

gpu_devices = tf.config.experimental.list_physical_devices('GPU')
for device in gpu_devices:
    tf.config.experimental.set_memory_growth(device, True)
    pass

def load_model_json(path_json, path_h5):
    json_file = open(path_json, 'r')
    loaded_model_json = json_file.read()
    json_file.close()
    loaded_model = model_from_json(loaded_model_json)
    # load weights into new model
    loaded_model.load_weights(path_h5)
    print("Loaded model from disk")
    return loaded_model

max_len = 60

vocabulary = ['', '-', '.', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

models = {}
for model_name in [ 'none', 'tld', 'icann', 'private' ]:
    models[model_name] = {
        'name': model_name,
        'nn': load_model_json(
            f'./nns/json_tf2.4/model_{model_name}.json',
            f'./nns/json_tf2.4/model_{model_name}.h5'
        )
    }
