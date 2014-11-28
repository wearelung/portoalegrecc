# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "mysql"
  s.version = "2.7"

  s.required_rubygems_version = nil if s.respond_to? :required_rubygems_version=
  s.autorequire = "mysql"
  s.bindir = nil
  s.cert_chain = nil
  s.date = "2005-10-09"
  s.email = "tommy@tmtm.org"
  s.extensions = ["extconf.rb"]
  s.files = ["extconf.rb"]
  s.homepage = "http://www.tmtm.org/en/mysql/ruby/"
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new("> 0.0.0")
  s.rubygems_version = "1.8.19"
  s.summary = "MySQL/Ruby provides the same functions for Ruby programs that the MySQL C API provides for C programs."

  if s.respond_to? :specification_version then
    s.specification_version = 1

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
