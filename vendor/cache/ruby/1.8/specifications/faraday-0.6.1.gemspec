# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "faraday"
  s.version = "0.6.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 1.3.5") if s.respond_to? :required_rubygems_version=
  s.authors = ["Rick Olson"]
  s.date = "2011-04-13"
  s.description = "HTTP/REST API client library."
  s.email = "technoweenie@gmail.com"
  s.homepage = "http://github.com/technoweenie/faraday"
  s.require_paths = ["lib"]
  s.rubyforge_project = "faraday"
  s.rubygems_version = "1.8.19"
  s.summary = "HTTP/REST API client library."

  if s.respond_to? :specification_version then
    s.specification_version = 2

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rake>, ["~> 0.8"])
      s.add_runtime_dependency(%q<addressable>, ["~> 2.2.4"])
      s.add_runtime_dependency(%q<multipart-post>, ["~> 1.1.0"])
      s.add_runtime_dependency(%q<rack>, ["< 2", ">= 1.1.0"])
    else
      s.add_dependency(%q<rake>, ["~> 0.8"])
      s.add_dependency(%q<addressable>, ["~> 2.2.4"])
      s.add_dependency(%q<multipart-post>, ["~> 1.1.0"])
      s.add_dependency(%q<rack>, ["< 2", ">= 1.1.0"])
    end
  else
    s.add_dependency(%q<rake>, ["~> 0.8"])
    s.add_dependency(%q<addressable>, ["~> 2.2.4"])
    s.add_dependency(%q<multipart-post>, ["~> 1.1.0"])
    s.add_dependency(%q<rack>, ["< 2", ">= 1.1.0"])
  end
end
