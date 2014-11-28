# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "akami"
  s.version = "1.0.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Daniel Harrington"]
  s.date = "2011-07-03"
  s.description = "Building Web Service Security"
  s.email = ["me@rubiii.com"]
  s.homepage = "https://github.com/rubiii/akami"
  s.require_paths = ["lib"]
  s.rubyforge_project = "akami"
  s.rubygems_version = "1.8.19"
  s.summary = "Web Service Security"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<gyoku>, [">= 0.4.0"])
      s.add_development_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_development_dependency(%q<rspec>, ["~> 2.5.0"])
      s.add_development_dependency(%q<mocha>, ["~> 0.9.8"])
      s.add_development_dependency(%q<timecop>, ["~> 0.3.5"])
      s.add_development_dependency(%q<autotest>, [">= 0"])
    else
      s.add_dependency(%q<gyoku>, [">= 0.4.0"])
      s.add_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_dependency(%q<rspec>, ["~> 2.5.0"])
      s.add_dependency(%q<mocha>, ["~> 0.9.8"])
      s.add_dependency(%q<timecop>, ["~> 0.3.5"])
      s.add_dependency(%q<autotest>, [">= 0"])
    end
  else
    s.add_dependency(%q<gyoku>, [">= 0.4.0"])
    s.add_dependency(%q<rake>, ["~> 0.8.7"])
    s.add_dependency(%q<rspec>, ["~> 2.5.0"])
    s.add_dependency(%q<mocha>, ["~> 0.9.8"])
    s.add_dependency(%q<timecop>, ["~> 0.3.5"])
    s.add_dependency(%q<autotest>, [">= 0"])
  end
end
