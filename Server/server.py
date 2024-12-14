import sqlite3
from flask import Flask, request, send_from_directory
from datetime import datetime
from telegram import Bot
from telegram.ext import Updater, MessageHandler, CallbackContext
from telegram.ext import filters
import requests


app = Flask(__name__)
DB_FILE = 'data.db'

incorrect_count = [0, 0, 0, 0]
max_incorrect = 0

# Initialize the database
def init_db():
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            password TEXT NOT NULL,
            bldg_number INTEGER NOT NULL
        )
    ''')
    conn.commit()
    conn.close()

# Helper function to connect to the database
def get_db_connection():
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    return conn

def get_chat_id(update: Updater, context: CallbackContext) -> None:
    chat_id = update.message.chat_id
    update.message.reply_text(f"Your Chat ID is: {chat_id}")
    print(f"Chat ID: {chat_id}")  # Logs the chat ID in the console

@app.route("/receive_data", methods=['POST'])
def receive_data():

    data = request.json
    # Extract query parameters
    password = str(data["password"])
    bldg_number = str(data["bldg_number"])

    print(password)
    print(bldg_number)
    # Validate the data
    if not (password and bldg_number):
        return "Missing one or more required parameters: password, bldg_number", 400
    
    chat_id = 7017630724
    bot_token = "7340323966:AAFfOhtRtH9Q83KQu8liIuZ8_eeEUFWs0KU"

    incorrect_count[int(bldg_number) - 1] += 0 if password == "True" else 1
    if incorrect_count[int(bldg_number) - 1] > max_incorrect:
        print("sending alert!")
        msg = f"Building number {bldg_number} has an intruder alert."
        send_message_via_requests(bot_token, chat_id, msg)
        incorrect_count[int(bldg_number) - 1] = 0

    # Generate the current timestamp in milliseconds
    timestamp = int(datetime.now().timestamp() * 1000)

    # Save the data to the database
    conn = get_db_connection()
    conn.execute(
        'INSERT INTO logs (timestamp, password, bldg_number) VALUES (?, ?, ?)',
        (timestamp, password, int(bldg_number))
    )
    conn.commit()
    conn.close()

    print(f"We received and saved the values: timestamp={timestamp}, password={password}, bldg_number={bldg_number}")
    # Respond to the client
    return f"We received and saved the values: timestamp={timestamp}, password={password}, bldg_number={bldg_number}"

@app.route("/get_data")
def get_data():
    # Connect to the database
    conn = get_db_connection()
    cursor = conn.cursor()
    
    # Fetch all data
    cursor.execute("SELECT * FROM logs")
    rows = cursor.fetchall()
    
    # Close the connection
    conn.close()

    # Convert rows to a list of dictionaries
    data = [
        {"id": row[0], "timestamp": row[1], "password": row[2], "bldg_number": row[3]}
        for row in rows
    ]
    
    # Return the data as JSON
    return {"logs": data}

@app.route("/")
def index():
    return send_from_directory('static', 'index.html')

def send_message_via_requests(token: str, chat_id: int, text: str):
    url = f"https://api.telegram.org/bot{token}/sendMessage"
    data = {"chat_id": chat_id, "text": text}
    
    try:
        response = requests.post(url, json=data)
        print("Response:", response.json())  # Print the response
    except Exception as e:
        print(f"Failed to send message via requests: {e}")

def send_message(token: str, chat_id: int, text: str):
    """
    Sends a message to a Telegram user or group.

    Args:
        token (str): The bot's API token from BotFather.
        chat_id (int): The unique ID of the chat (user or group).
        text (str): The message to send.
    """
    try:
        # Initialize the bot
        bot = Bot(token=token)
        
        # Send the message
        bot.send_message(chat_id=chat_id, text=text)
        print(f"Message sent to chat ID {chat_id}: {text}")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == '__main__':
    init_db()
    chat_id = 7017630724
    # bot_token = "7340323966:AAFfOhtRtH9Q83KQu8liIuZ8_eeEUFWs0KU"
    # msg = "Startedstared"
    # try:
    #     print("Attempting to send message...")
    #     send_message_via_requests(bot_token, chat_id, msg)
    #     print("Message function executed successfully.")
    # except Exception as e:
    #     print(f"Failed to send message: {e}")
    app.run(host='0.0.0.0', port=3000)
    
    # bot_token = "7340323966:AAFfOhtRtH9Q83KQu8liIuZ8_eeEUFWs0KU"
    # updater = Updater(bot_token)
    # dp = updater.dispatcher

    # dp.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, get_chat_id))

    # updater.start_polling()
    # updater.idle()