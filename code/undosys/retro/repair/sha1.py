import hmac, hashlib

def sha1(data):
        if len(data) == 0:
                return ""
        return hmac.new("retro", data, hashlib.sha1).digest()

def hexdigest(sha):
        return sha.encode("hex")
