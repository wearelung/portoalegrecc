# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "acts_as_reportable"
  s.version = "1.1.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Michael Milner"]
  s.date = "2008-06-09"
  s.description = "acts_as_reportable provides ActiveRecord support for Ruby Reports"
  s.email = "mikem836@gmail.com"
  s.homepage = "http://rubyreports.org"
  s.rdoc_options = ["--title", "Ruport Documentation"]
  s.require_paths = ["lib"]
  s.rubyforge_project = "ruport"
  s.rubygems_version = "1.8.19"
  s.summary = "ActiveRecord support for Ruby Reports"

  if s.respond_to? :specification_version then
    s.specification_version = 2

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<ruport>, [">= 1.6.0"])
    else
      s.add_dependency(%q<ruport>, [">= 1.6.0"])
    end
  else
    s.add_dependency(%q<ruport>, [">= 1.6.0"])
  end
end
