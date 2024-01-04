class DataBoundary:
     def __init__(self):
        self.TINYINT_BOUNDARY = [-128, 127]
        self.SMALLINT_BOUNDARY = [-32768, 32767]
        self.INT_BOUNDARY = [-2147483648, 2147483647]
        self.BIGINT_BOUNDARY = [-9223372036854775808, 9223372036854775807]
        self.UTINYINT_BOUNDARY = [0, 255]
        self.USMALLINT_BOUNDARY = [0, 65535]
        self.UINT_BOUNDARY = [0, 4294967295]
        self.UBIGINT_BOUNDARY = [0, 18446744073709551615]
        self.FLOAT_BOUNDARY = [-3.40E+38, 3.40E+38]
        self.DOUBLE_BOUNDARY = [-1.7e+308, 1.7e+308]
        self.BOOL_BOUNDARY = [True, False]
        self.BINARY_MAX_LENGTH = 16374
        self.NCHAR_MAX_LENGTH = 4093
        self.DBNAME_MAX_LENGTH = 64
        self.STBNAME_MAX_LENGTH = 192
        self.TBNAME_MAX_LENGTH = 192
        self.CHILD_TBNAME_MAX_LENGTH = 192
        self.TAG_KEY_MAX_LENGTH = 64
        self.COL_KEY_MAX_LENGTH = 64
        self.MAX_TAG_COUNT = 128
        self.MAX_TAG_COL_COUNT = 4096
        self.mnodeShmSize = [6292480, 2147483647]
        self.mnodeShmSize_default = 6292480
        self.vnodeShmSize = [6292480, 2147483647]
        self.vnodeShmSize_default = 31458304
        self.DB_PARAM_BUFFER_CONFIG = {"create_name": "buffer", "query_name": "buffer", "vnode_json_key": "szBuf", "boundary": [3, 16384], "default": 96}
        self.DB_PARAM_CACHELAST_CONFIG = {"create_name": "cachelast", "query_name": "cache_model", "vnode_json_key": "", "boundary": [0, 1, 2, 3], "default": 0}
        self.DB_PARAM_COMP_CONFIG = {"create_name": "comp", "query_name": "compression", "vnode_json_key": "", "boundary": [0, 1, 2], "default": 2}
        self.DB_PARAM_DURATION_CONFIG = {"create_name": "duration", "query_name": "duration", "vnode_json_key": "daysPerFile", "boundary": [1, 3650, '60m', '5256000m', '1h', '87600h', '1d', '3650d'], "default": "14400m"}
        self.DB_PARAM_FSYNC_CONFIG = {"create_name": "fsync", "query_name": "fsync", "vnode_json_key": "", "boundary": [0, 180000], "default": 3000}
        self.DB_PARAM_KEEP_CONFIG = {"create_name": "keep", "query_name": "fsync", "vnode_json_key": "", "boundary": [1, 365000,'1440m','525600000m','24h','8760000h','1d','365000d'], "default": "5256000m,5256000m,5256000m"}
        self.DB_PARAM_MAXROWS_CONFIG = {"create_name": "maxrows", "query_name": "maxrows", "vnode_json_key": "maxRows", "boundary": [200, 10000], "default": 4096}
        self.DB_PARAM_MINROWS_CONFIG = {"create_name": "minrows", "query_name": "minrows", "vnode_json_key": "minRows", "boundary": [10, 1000], "default": 100}
        self.DB_PARAM_NTABLES_CONFIG = {"create_name": "ntables", "query_name": "ntables", "vnode_json_key": "", "boundary": 0, "default": 0}
        self.DB_PARAM_PAGES_CONFIG = {"create_name": "pages", "query_name": "pages", "vnode_json_key": "szCache", "boundary": [64], "default": 256}
        self.DB_PARAM_PAGESIZE_CONFIG = {"create_name": "pagesize", "query_name": "pagesize", "vnode_json_key": "szPage", "boundary": [1, 16384], "default": 4}
        self.DB_PARAM_PRECISION_CONFIG = {"create_name": "precision", "query_name": "precision", "vnode_json_key": "", "boundary": ['ms', 'us', 'ns'], "default": "ms"}
        self.DB_PARAM_REPLICA_CONFIG = {"create_name": "replica", "query_name": "replica", "vnode_json_key": "", "boundary": [1], "default": 1}
        self.DB_PARAM_SINGLE_STABLE_CONFIG = {"create_name": "single_stable", "query_name": "single_stable_model", "vnode_json_key": "", "boundary": [0, 1], "default": 0}
        self.DB_PARAM_STRICT_CONFIG = {"create_name": "strict", "query_name": "strict", "vnode_json_key": "", "boundary": {"off": 0, "strict": 1}, "default": "off"}
        self.DB_PARAM_VGROUPS_CONFIG = {"create_name": "vgroups", "query_name": "vgroups", "vnode_json_key": "", "boundary": [1, 32], "default": 2}
        self.DB_PARAM_WAL_CONFIG = {"create_name": "wal", "query_name": "wal", "vnode_json_key": "", "boundary": [1, 2], "default": 1}