from enum import Enum


class ServerStatusCode(Enum):
    UNKNOWN_ERROR = "1"
    INVALID_REQUEST = "2"
    USER_NOT_FOUND = "3"
    USERNAME_TAKEN = "4"
    INVALID_MESSAGE_FORMAT = "5"
    RATE_LIMIT_EXCEEDED = "6"
    CONNECTION_ERROR = "7"
    TIMEOUT = "8"

    SUCCESS = "200"
