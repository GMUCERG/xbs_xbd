import hashlib

BLOCKSIZE=1024*1024

def sha256_file(filename):
    with open(filename,mode='rb') as f:
        h = hashlib.sha256()
        block = f.read(BLOCKSIZE)
        while len(block) != 0:
            h.update(block)
            block = f.read(BLOCKSIZE)
        
        return h.hexdigest()



def hash_obj(obj):
    h = hashlib.sha256()
    objs = {}
    #items = []

    def trav_item(name, item):
        value = None
        if id(item) in objs.keys():
            value="ref: "+'.'.join(objs[id(item)])
        else:
            objs[id(item)]=name

            if hasattr(item, "__iter__") and not isinstance(item, str):
                if hasattr(item, "items"):
                    for k,v in sorted(item.items()):
                        trav_item(name+(k,), v)
                else:
                    c = 0
                    for i in sorted(item):
                        trav_item(name+(str(c),), i)
                        c += 1

            elif hasattr(item, "__dict__"): 
                for k, v in sorted(item.__dict__.items()):
                    trav_item(name+(k,), v)
            else:
                value = item
        if value != None:
            value_str = "/".join(name)+":"+str(value)
            h.update(value_str.encode())
            #items.append(value_str)

    trav_item(("/",),obj)

    return h.hexdigest()

# Gets db path before config is initialized, since config initialization depends
# on database
def get_db_path(config_path):
    import configparser
    config = configparser.ConfigParser()
    config.read(config_path)
    return config.get('paths','data')

