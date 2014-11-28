# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "nori"
  s.version = "1.1.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Daniel Harrington", "John Nunemaker", "Wynn Netherland"]
  s.date = "2012-02-17"
  s.description = "XML to Hash translator"
  s.email = "me@rubiii.com"
  s.homepage = "http://github.com/rubiii/nori"
  s.require_paths = ["lib"]
  s.rubyforge_project = "nori"
  s.rubygems_version = "1.8.19"
  s.summary = "XML to Hash translator"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_development_dependency(%q<nokogiri>, [">= 1.4.0"])
      s.add_development_dependency(%q<rspec>, ["~> 2.5.0"])
      s.add_development_dependency(%q<autotest>, [">= 0"])
    else
      s.add_dependency(%q<rake>, ["~> 0.8.7"])
      s.add_dependency(%q<nokogiri>, [">= 1.4.0"])
      s.add_dependency(%q<rspec>, ["~> 2.5.0"])
      s.add_dependency(%q<autotest>, [">= 0"])
    end
  else
    s.add_dependency(%q<rake>, ["~> 0.8.7"])
    s.add_dependency(%q<nokogiri>, [">= 1.4.0"])
    s.add_dependency(%q<rspec>, ["~> 2.5.0"])
    s.add_dependency(%q<autotest>, [">= 0"])
  end
end
