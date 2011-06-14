import copy, logging

BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RESERVED, DEFAULT = range(10)

COLORS = {
    "DEBUG"   : DEFAULT,
    "INFO"    : DEFAULT,
    "WARNING" : YELLOW,
    "CRITICAL": RED,
    "ERROR"   : RED,
}

class ColoredFormatter(logging.Formatter):
    def format(self, rec):
	m = logging.Formatter.format(self, rec)
	color = COLORS[rec.levelname]
	if color == DEFAULT:
	    return m
	return "\033[3%dm" % color + m + "\033[m"

def coloredConfig(**kwargs):
    logger = logging.getLogger()
    if len(logger.handlers) > 0:
	return
    h = logging.StreamHandler(kwargs.get("stream"))
    fs = kwargs.get("format", logging.BASIC_FORMAT)
    if kwargs.get("color", True):
	fmt = ColoredFormatter(fs)
    else:
	fmt = logging.Formatter(fs)
    h.setFormatter(fmt)
    logger.setLevel(kwargs.get("level", logging.INFO))
    logger.addHandler(h)
