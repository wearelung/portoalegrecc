# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "gyoku"
  s.version = "0.4.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Daniel Harrington"]
  s.date = "2011-05-15"
  s.description = "Gyoku converts Ruby Hashes to XML"
  s.email = "me@rubiii.com"
  s.homepage = "http://github.com/rubiii/gyoku"
  s.require_paths = ["lib"]
  s.rubyforge_project = "gyoku"
  s.rubygems_version = "1.8.19"
  s.summary = "Converts Ruby Hashes to XML"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<builder>, [">= 2.1.2"])
      s.add_development_dependency(%q<rspec>, ["~> 2.4.0"])
      s.add_development_dependency(%q<autotest>, [">= 0"])
      s.add_development_dependency(%q<mocha>, ["~> 0.9.9"])
    else
      s.add_dependency(%q<builder>, [">= 2.1.2"])
      s.add_dependency(%q<rspec>, ["~> 2.4.0"])
      s.add_dependency(%q<autotest>, [">= 0"])
      s.add_dependency(%q<mocha>, ["~> 0.9.9"])
    end
  else
    s.add_dependency(%q<builder>, [">= 2.1.2"])
    s.add_dependency(%q<rspec>, ["~> 2.4.0"])
    s.add_dependency(%q<autotest>, [">= 0"])
    s.add_dependency(%q<mocha>, ["~> 0.9.9"])
  end
end
