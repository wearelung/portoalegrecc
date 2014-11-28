# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "pyu-ruby-sasl"
  s.version = "0.0.3.3"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Stephan Maka", "Ping Yu"]
  s.date = "2010-10-18"
  s.description = "Simple Authentication and Security Layer (RFC 4422)"
  s.email = "pyu@intridea.com"
  s.homepage = "http://github.com/pyu10055/ruby-sasl/"
  s.require_paths = ["lib"]
  s.rubygems_version = "1.8.19"
  s.summary = "SASL client library"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
