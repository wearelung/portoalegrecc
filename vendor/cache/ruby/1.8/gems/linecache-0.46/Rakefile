#!/usr/bin/env rake
# -*- Ruby -*-
require 'rubygems'
require 'rake/gempackagetask'
require 'rake/rdoctask'
require 'rake/testtask'

SO_NAME = 'trace_nums.so'

ROOT_DIR = File.dirname(__FILE__)
load File.join %W(#{ROOT_DIR} lib version.rb)

PKG_VERSION = LineCache::VERSION
PKG_NAME           = 'linecache'
PKG_FILE_NAME      = "#{PKG_NAME}-#{PKG_VERSION}"
RUBY_FORGE_PROJECT = 'rocky-hacks'
RUBY_FORGE_USER    = 'rockyb'

FILES = FileList[
  'AUTHORS',
  'COPYING',
  'ChangeLog',
  'NEWS',
  'README',
  'Rakefile',
  'ext/trace_nums.c',
  'ext/trace_nums.h',
  'ext/extconf.rb',
  'lib/*.rb',
  'test/*.rb',
  'test/data/*.rb',
  'test/short-file'
]                        

desc 'Test everything'
Rake::TestTask.new(:test) do |t|
  t.libs << './lib'
  t.pattern = 'test/test-*.rb'
  t.options = '--verbose' if $VERBOSE
end
task :test => :lib

desc 'Create the core ruby-debug shared library extension'
task :lib do
  Dir.chdir('ext') do
    system("#{Gem.ruby} extconf.rb && make")
  end
end


desc 'Test everything - same as test.'
task :check => :test

desc 'Create a GNU-style ChangeLog via svn2cl'
task :ChangeLog do
  system('svn2cl --authors=svn2cl_usermap')
end

gem_file = nil

# Base GEM Specification
default_spec = Gem::Specification.new do |spec|
  spec.name = 'linecache'
  
  spec.homepage = 'http://rubyforge.org/projects/rocky-hacks/linecache'
  spec.summary = 'Read file with caching'
  spec.description = <<-EOF
LineCache is a module for reading and caching lines. This may be useful for
example in a debugger where the same lines are shown many times.
EOF

  spec.version = PKG_VERSION

  spec.author = 'R. Bernstein'
  spec.email = 'rockyb@rubyforge.net'
  spec.platform = Gem::Platform::RUBY
  spec.require_path = 'lib'
  spec.files = FILES.to_a  
  spec.add_dependency('rbx-require-relative', '> 0.0.4')
  spec.extensions = ['ext/extconf.rb']

  spec.required_ruby_version = '>= 1.8.7'
  spec.date = Time.now
  spec.rubyforge_project = 'rocky-hacks'
  
  # rdoc
  spec.has_rdoc = true
  spec.extra_rdoc_files = ['README', 'lib/linecache.rb', 'lib/tracelines.rb']

  spec.test_files = FileList['test/*.rb']
  gem_file = "#{spec.name}-#{spec.version}.gem"

end

# Rake task to build the default package
Rake::GemPackageTask.new(default_spec) do |pkg|
  pkg.need_tar = true
end

task :default => [:test]

# Windows specification
win_spec = default_spec.clone
win_spec.extensions = []
## win_spec.platform = Gem::Platform::WIN32 # deprecated
win_spec.platform = 'mswin32'
win_spec.files += ["lib/#{SO_NAME}"]
win_spec.add_dependency('rbx-require-relative', '> 0.0.4')

desc 'Create Windows Gem'
task :win32_gem do
  # Copy the win32 extension the top level directory.
  current_dir = File.expand_path(File.dirname(__FILE__))
  source = File.join(current_dir, 'ext', 'win32', SO_NAME)
  target = File.join(current_dir, 'lib', SO_NAME)
  cp(source, target)

  # Create the gem, then move it to pkg.
  Gem::Builder.new(win_spec).build
  gem_file = "#{win_spec.name}-#{win_spec.version}-#{win_spec.platform}.gem"
  mv(gem_file, "pkg/#{gem_file}")

  # Remove win extension from top level directory.
  rm(target)
end

desc 'Publish linecache to RubyForge.'
task :publish do 
  require 'rake/contrib/sshpublisher'
  
  # Get ruby-debug path.
  ruby_debug_path = File.expand_path(File.dirname(__FILE__))

  publisher = Rake::SshDirPublisher.new('rockyb@rubyforge.org',
        '/var/www/gforge-projects/rocky-hacks/linecache', ruby_debug_path)
end

desc 'Remove residue from running patch'
task :rm_patch_residue do
  FileUtils.rm_rf Dir.glob('**/*.{rej,orig}'), :verbose => true
end

desc 'Remove ~ backup files'
task :rm_tilde_backups do
  FileUtils.rm_rf Dir.glob('**/*~'), :verbose => true
end

desc 'Remove built files'
task :clean => [:clobber_package, :clobber_rdoc, :rm_patch_residue, 
                :rm_tilde_backups] do
  cd 'ext' do
    if File.exists?('Makefile')
      sh 'make clean'
      rm 'Makefile'
    end
    derived_files = Dir.glob('.o') + Dir.glob('*.so')
    rm derived_files unless derived_files.empty?
  end
end

# ---------  RDoc Documentation ------
desc 'Generate rdoc documentation'
Rake::RDocTask.new('rdoc') do |rdoc|
  rdoc.rdoc_dir = 'doc'
  rdoc.title    = "linecache #{LineCache::VERSION} Documentation"
  rdoc.options << '--main' << 'README'
  rdoc.rdoc_files.include('ext/**/*.c',
                          'lib/*.rb', 
                          'README', 
                          'COPYING')
end

desc 'Publish the release files to RubyForge.'
task :rubyforge_upload do
  `rubyforge login`
  release_command = "rubyforge add_release #{PKG_NAME} #{PKG_NAME} '#{PKG_NAME}-#{PKG_VERSION}' pkg/#{PKG_NAME}-#{PKG_VERSION}.gem"
  puts release_command
  system(release_command)
end

desc 'Install the gem locally'
task :install => :gem do
  Dir.chdir(ROOT_DIR) do
    sh %{gem install --local pkg/#{gem_file}}
  end
end

