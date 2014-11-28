# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "linecache"
  s.version = "0.46"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["R. Bernstein"]
  s.date = "2011-06-19"
  s.description = "LineCache is a module for reading and caching lines. This may be useful for\nexample in a debugger where the same lines are shown many times.\n"
  s.email = "rockyb@rubyforge.net"
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = ["README", "lib/linecache.rb", "lib/tracelines.rb"]
  s.files = ["README", "lib/linecache.rb", "lib/tracelines.rb", "ext/extconf.rb"]
  s.homepage = "http://rubyforge.org/projects/rocky-hacks/linecache"
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new(">= 1.8.7")
  s.rubyforge_project = "rocky-hacks"
  s.rubygems_version = "1.8.19"
  s.summary = "Read file with caching"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<rbx-require-relative>, ["> 0.0.4"])
    else
      s.add_dependency(%q<rbx-require-relative>, ["> 0.0.4"])
    end
  else
    s.add_dependency(%q<rbx-require-relative>, ["> 0.0.4"])
  end
end
