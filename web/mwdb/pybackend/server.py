from flask import Flask, request, jsonify
from flask_cors import CORS
from defs import Database, SlotConfig
from plot import Plot

app = Flask(__name__)

CORS(app)

db = Database()

@app.route('/dataset/plot', methods=['POST'])
def get_data():
    data = request.get_json()
    config = SlotConfig(**data)
    plot = Plot(db, config)
    svg, df = plot.bydga()
    # svg, df = plot.bydga()
    return jsonify({
        "svg": svg,
        "df": df.to_json(orient="columns")
    })

if __name__ == '__main__':
    app.run(port=5002)