# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "mailfactory"
  s.version = "1.4.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["David Powers"]
  s.date = "2008-08-06"
  s.description = "MailFactory is s simple module for producing RFC compliant mail that can include multiple attachments, multiple body parts, and arbitrary headers"
  s.email = "david@grayskies.net"
  s.homepage = "http://mailfactory.rubyforge.org"
  s.require_paths = ["lib"]
  s.rubyforge_project = "mailfactory"
  s.rubygems_version = "1.8.19"
  s.summary = "MailFactory is a pure-ruby MIME mail generator"

  if s.respond_to? :specification_version then
    s.specification_version = 2

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<mime-types>, [">= 1.13.1"])
    else
      s.add_dependency(%q<mime-types>, [">= 1.13.1"])
    end
  else
    s.add_dependency(%q<mime-types>, [">= 1.13.1"])
  end
end
