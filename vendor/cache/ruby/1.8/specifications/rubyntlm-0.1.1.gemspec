# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "rubyntlm"
  s.version = "0.1.1"

  s.required_rubygems_version = nil if s.respond_to? :required_rubygems_version=
  s.authors = ["Kohei Kajimoto"]
  s.autorequire = "net/ntlm"
  s.cert_chain = nil
  s.date = "2006-10-04"
  s.description = "Ruby/NTLM provides message creator and parser for the NTLM authentication."
  s.email = "koheik@gmail.com"
  s.extra_rdoc_files = ["README"]
  s.files = ["README"]
  s.homepage = "http://rubyforge.org/projects/rubyntlm"
  s.rdoc_options = ["--main", "README"]
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new("> 0.0.0")
  s.rubyforge_project = "rubyntlm"
  s.rubygems_version = "1.8.19"
  s.summary = "Ruby/NTLM library."

  if s.respond_to? :specification_version then
    s.specification_version = 1

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
