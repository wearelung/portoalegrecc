# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "httpi"
  s.version = "0.9.7"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Daniel Harrington", "Martin Tepper"]
  s.date = "2012-04-26"
  s.description = "HTTPI provides a common interface for Ruby HTTP libraries."
  s.email = "me@rubiii.com"
  s.homepage = "http://github.com/rubiii/httpi"
  s.require_paths = ["lib"]
  s.rubyforge_project = "httpi"
  s.rubygems_version = "1.8.19"
  s.summary = "Interface for Ruby HTTP libraries"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<rack>, [">= 0"])
      s.add_development_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_development_dependency(%q<rspec>, ["~> 2.7"])
      s.add_development_dependency(%q<mocha>, ["~> 0.9.9"])
      s.add_development_dependency(%q<webmock>, ["~> 1.4.0"])
      s.add_development_dependency(%q<autotest>, [">= 0"])
      s.add_development_dependency(%q<ZenTest>, ["= 4.5.0"])
    else
      s.add_dependency(%q<rack>, [">= 0"])
      s.add_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_dependency(%q<rspec>, ["~> 2.7"])
      s.add_dependency(%q<mocha>, ["~> 0.9.9"])
      s.add_dependency(%q<webmock>, ["~> 1.4.0"])
      s.add_dependency(%q<autotest>, [">= 0"])
      s.add_dependency(%q<ZenTest>, ["= 4.5.0"])
    end
  else
    s.add_dependency(%q<rack>, [">= 0"])
    s.add_dependency(%q<rake>, ["~> 0.8.7"])
    s.add_dependency(%q<rspec>, ["~> 2.7"])
    s.add_dependency(%q<mocha>, ["~> 0.9.9"])
    s.add_dependency(%q<webmock>, ["~> 1.4.0"])
    s.add_dependency(%q<autotest>, [">= 0"])
    s.add_dependency(%q<ZenTest>, ["= 4.5.0"])
  end
end
