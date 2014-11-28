# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "ruby-debug-base"
  s.version = "0.10.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Kent Sibilev"]
  s.date = "2010-10-27"
  s.description = "ruby-debug is a fast implementation of the standard Ruby debugger debug.rb.\nIt is implemented by utilizing a new Ruby C API hook. The core component \nprovides support that front-ends can build on. It provides breakpoint \nhandling, bindings for stack frames among other things.\n"
  s.email = "ksibilev@yahoo.com"
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = ["README", "ext/ruby_debug.c"]
  s.files = ["README", "ext/ruby_debug.c", "ext/extconf.rb"]
  s.homepage = "http://rubyforge.org/projects/ruby-debug/"
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new(">= 1.8.2")
  s.rubyforge_project = "ruby-debug"
  s.rubygems_version = "1.8.19"
  s.summary = "Fast Ruby debugger - core component"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<linecache>, [">= 0.3"])
    else
      s.add_dependency(%q<linecache>, [">= 0.3"])
    end
  else
    s.add_dependency(%q<linecache>, [">= 0.3"])
  end
end
