# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "ruby-debug"
  s.version = "0.10.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Kent Sibilev"]
  s.date = "2010-10-27"
  s.description = "A generic command line interface for ruby-debug.\n"
  s.email = "ksibilev@yahoo.com"
  s.executables = ["rdebug"]
  s.extra_rdoc_files = ["README"]
  s.files = ["bin/rdebug", "README"]
  s.homepage = "http://rubyforge.org/projects/ruby-debug/"
  s.require_paths = ["cli"]
  s.required_ruby_version = Gem::Requirement.new(">= 1.8.2")
  s.rubyforge_project = "ruby-debug"
  s.rubygems_version = "1.8.19"
  s.summary = "Command line interface (CLI) for ruby-debug-base"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<columnize>, [">= 0.1"])
      s.add_runtime_dependency(%q<ruby-debug-base>, ["~> 0.10.4.0"])
    else
      s.add_dependency(%q<columnize>, [">= 0.1"])
      s.add_dependency(%q<ruby-debug-base>, ["~> 0.10.4.0"])
    end
  else
    s.add_dependency(%q<columnize>, [">= 0.1"])
    s.add_dependency(%q<ruby-debug-base>, ["~> 0.10.4.0"])
  end
end
