srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.check(lib='mp3splt', uselib_store='mp3splt', mandatory=True)
  conf.env.append_value("LIB_MP3SPLT","mp3splt")


def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.uselib = "MP3SPLT"
  obj.target = 'binding'
  obj.source = 'binding.cc'
