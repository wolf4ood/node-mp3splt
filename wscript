srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'
import Options

def set_options(opt):
  opt.tool_options('compiler_cxx')
  opt.add_option('--debug', action='store_true', default=False, dest='debug',help='Debug mode')


def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.check(lib='mp3splt', uselib_store='mp3splt', mandatory=True)
  conf.env.append_value("LIB_MP3SPLT","mp3splt")
  if Options.options.debug:
  	conf.env['CFLAGS'] = ['-O0', '-g3']


def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.uselib = "MP3SPLT"
  obj.target = 'binding'
  obj.source = 'binding.cc'
