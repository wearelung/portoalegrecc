# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "savon"
  s.version = "0.9.9"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Daniel Harrington"]
  s.date = "2012-02-17"
  s.description = "Ruby's heavy metal SOAP client"
  s.email = "me@rubiii.com"
  s.homepage = "http://savonrb.com"
  s.require_paths = ["lib"]
  s.rubyforge_project = "savon"
  s.rubygems_version = "1.8.19"
  s.summary = "Heavy metal Ruby SOAP client"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<builder>, [">= 2.1.2"])
      s.add_runtime_dependency(%q<nori>, ["~> 1.1"])
      s.add_runtime_dependency(%q<httpi>, ["~> 0.9"])
      s.add_runtime_dependency(%q<wasabi>, ["~> 2.1"])
      s.add_runtime_dependency(%q<akami>, ["~> 1.0"])
      s.add_runtime_dependency(%q<gyoku>, [">= 0.4.0"])
      s.add_runtime_dependency(%q<nokogiri>, [">= 1.4.0"])
      s.add_development_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_development_dependency(%q<rspec>, ["~> 2.5.0"])
      s.add_development_dependency(%q<mocha>, ["~> 0.9.8"])
      s.add_development_dependency(%q<timecop>, ["~> 0.3.5"])
      s.add_development_dependency(%q<autotest>, [">= 0"])
      s.add_development_dependency(%q<ZenTest>, ["= 4.5.0"])
    else
      s.add_dependency(%q<builder>, [">= 2.1.2"])
      s.add_dependency(%q<nori>, ["~> 1.1"])
      s.add_dependency(%q<httpi>, ["~> 0.9"])
      s.add_dependency(%q<wasabi>, ["~> 2.1"])
      s.add_dependency(%q<akami>, ["~> 1.0"])
      s.add_dependency(%q<gyoku>, [">= 0.4.0"])
      s.add_dependency(%q<nokogiri>, [">= 1.4.0"])
      s.add_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_dependency(%q<rspec>, ["~> 2.5.0"])
      s.add_dependency(%q<mocha>, ["~> 0.9.8"])
      s.add_dependency(%q<timecop>, ["~> 0.3.5"])
      s.add_dependency(%q<autotest>, [">= 0"])
      s.add_dependency(%q<ZenTest>, ["= 4.5.0"])
    end
  else
    s.add_dependency(%q<builder>, [">= 2.1.2"])
    s.add_dependency(%q<nori>, ["~> 1.1"])
    s.add_dependency(%q<httpi>, ["~> 0.9"])
    s.add_dependency(%q<wasabi>, ["~> 2.1"])
    s.add_dependency(%q<akami>, ["~> 1.0"])
    s.add_dependency(%q<gyoku>, [">= 0.4.0"])
    s.add_dependency(%q<nokogiri>, [">= 1.4.0"])
    s.add_dependency(%q<rake>, ["~> 0.8.7"])
    s.add_dependency(%q<rspec>, ["~> 2.5.0"])
    s.add_dependency(%q<mocha>, ["~> 0.9.8"])
    s.add_dependency(%q<timecop>, ["~> 0.3.5"])
    s.add_dependency(%q<autotest>, [">= 0"])
    s.add_dependency(%q<ZenTest>, ["= 4.5.0"])
  end
end
