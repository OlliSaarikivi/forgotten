class Table:
    declaredIndices = []
    def __init__(self, name, columns, key):
        self.name = name
        self.columns = columns
        self.key = key

tables = {}
def table(name, *args, key='id'):
    tables[name] = Table(name, args, key)

class View:
    declaredIndices = []
    def __init__(self, name, columns, query):
        self.name = name
        self.columns = columns
        self.query = query
        
views = {}
def view(name, *args):
    views[name] = View(name, args[:-1], args[-1])

def index(target, key):
    if target in tables:
        tables[target].declaredIndices.append(key)
    if target in views:
        views[target].declaredIndices.append(key)
    
# Queries

queries = {}
def query(name, queryDef):
    queries[name] = queryDef

class MergeJoin:
    def __init__(self, leftIndex, rightIndex):
        self.leftIndex = leftIndex
        self.rightIndex = rightIndex

def merge_join(leftIndex, rightIndex):
    return MergeJoin(leftIndex, rightIndex)

def merge_join_using(key, left, right):
    return merge_join((left, key), (right, key))

class Alias:
    def __init__(self, target, name):
        self.target = target
        self.name = name

def alias(target, name):
    return Alias(target, name)
