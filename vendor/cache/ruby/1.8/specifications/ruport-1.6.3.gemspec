# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "ruport"
  s.version = "1.6.3"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Gregory Brown", "Mike Milner", "Andrew France"]
  s.date = "2009-12-12"
  s.description = "      Ruby Reports is a software library that aims to make the task of reporting\n      less tedious and painful. It provides tools for data acquisition,\n      database interaction, formatting, and parsing/munging.\n"
  s.email = "gregory.t.brown@gmail.com"
  s.extra_rdoc_files = ["LICENSE", "README"]
  s.files = ["LICENSE", "README"]
  s.homepage = "http://rubyreports.org"
  s.rdoc_options = ["--title", "Ruport Documentation", "--main", "README", "-q"]
  s.require_paths = ["lib"]
  s.rubyforge_project = "ruport"
  s.rubygems_version = "1.8.19"
  s.summary = "A generalized Ruby report generation and templating engine."

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<fastercsv>, [">= 0"])
      s.add_runtime_dependency(%q<pdf-writer>, ["= 1.1.8"])
    else
      s.add_dependency(%q<fastercsv>, [">= 0"])
      s.add_dependency(%q<pdf-writer>, ["= 1.1.8"])
    end
  else
    s.add_dependency(%q<fastercsv>, [">= 0"])
    s.add_dependency(%q<pdf-writer>, ["= 1.1.8"])
  end
end
