# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "transaction-simple"
  s.version = "1.4.0"

  s.required_rubygems_version = nil if s.respond_to? :required_rubygems_version=
  s.authors = ["Austin Ziegler"]
  s.cert_chain = nil
  s.date = "2007-02-03"
  s.description = "Transaction::Simple provides a generic way to add active transaction support to objects. The transaction methods added by this module will work with most objects, excluding those that cannot be Marshal-ed (bindings, procedure objects, IO instances, or singleton objects)."
  s.email = "austin@rubyforge.org"
  s.extra_rdoc_files = ["History.txt", "Install.txt", "Licence.txt", "Readme.txt"]
  s.files = ["History.txt", "Install.txt", "Licence.txt", "Readme.txt"]
  s.homepage = "http://rubyforge.org/projects/trans-simple"
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new(">= 1.8.2")
  s.rubyforge_project = "trans-simple"
  s.rubygems_version = "1.8.19"
  s.summary = "Simple object transaction support for Ruby."

  if s.respond_to? :specification_version then
    s.specification_version = 1

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<hoe>, [">= 1.1.7"])
    else
      s.add_dependency(%q<hoe>, [">= 1.1.7"])
    end
  else
    s.add_dependency(%q<hoe>, [">= 1.1.7"])
  end
end
