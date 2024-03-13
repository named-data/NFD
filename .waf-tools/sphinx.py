# inspired by code by Hans-Martin von Gaudecker, 2012

"""Support for Sphinx documentation"""

import os
from waflib import Task, TaskGen


class sphinx_build(Task.Task):
    color = 'BLUE'
    run_str = '${SPHINX_BUILD} -q -b ${BUILDERNAME} -D ${VERSION} -D ${RELEASE} -d ${DOCTREEDIR} ${SRCDIR} ${OUTDIR}'

    def keyword(self):
        return f'Processing ({self.env.BUILDERNAME})'


# from https://docs.python.org/3.12/whatsnew/3.12.html#imp
def load_source(modname, filename):
    import importlib.util
    from importlib.machinery import SourceFileLoader
    loader = SourceFileLoader(modname, filename)
    spec = importlib.util.spec_from_file_location(modname, filename, loader=loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


@TaskGen.feature('sphinx')
@TaskGen.before_method('process_source')
def process_sphinx(self):
    """Set up the task generator with a Sphinx instance and create a task."""

    conf = self.path.find_node(self.config)
    if not conf:
        self.bld.fatal(f'Sphinx configuration file {repr(self.config)} not found')

    inputs = [conf] + self.to_nodes(self.source)
    task = self.create_task('sphinx_build', inputs, always_run=getattr(self, 'always', False))

    confdir = conf.parent.abspath()
    buildername = getattr(self, 'builder', 'html')
    srcdir = getattr(self, 'srcdir', confdir)
    outdir = self.path.find_or_declare(getattr(self, 'outdir', buildername)).get_bld()
    doctreedir = getattr(self, 'doctreedir', os.path.join(outdir.abspath(), '.doctrees'))
    release = getattr(self, 'release', self.version)

    task.env['BUILDERNAME'] = buildername
    task.env['SRCDIR'] = srcdir
    task.env['OUTDIR'] = outdir.abspath()
    task.env['DOCTREEDIR'] = doctreedir
    task.env['VERSION'] = f'version={self.version}'
    task.env['RELEASE'] = f'release={release}'

    if buildername == 'man':
        confdata = load_source('sphinx_conf', conf.abspath())
        for i in confdata.man_pages:
            target = outdir.find_or_declare(f'{i[1]}.{i[4]}')
            task.outputs.append(target)
            if self.install_path:
                self.bld.install_files(f'{self.install_path}/man{i[4]}/', target)
    else:
        task.outputs.append(outdir)

    # prevent process_source from complaining that there is no extension mapping for .rst files
    self.source = []


def configure(conf):
    """Check if sphinx-build program is available."""
    conf.find_program('sphinx-build', var='SPHINX_BUILD', mandatory=False)


# sphinx command
from waflib.Build import BuildContext
class sphinx(BuildContext):
    cmd = 'sphinx'
    fun = 'sphinx'
