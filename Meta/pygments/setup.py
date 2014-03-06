from setuptools import setup, find_packages

setup (
	name = 'fablexer',
	packages = find_packages(),
	entry_points =
	"""
	[pygments.lexers]
	fablexer = fablexer.lexer:FabLexer
	""",
)
