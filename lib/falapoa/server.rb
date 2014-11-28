require File.join(File.dirname(__FILE__), "..", "../lib/falapoa/bootstrap.rb")
require File.join(File.dirname(__FILE__), "..", "../lib/falapoa/consumer.rb")
require File.join(File.dirname(__FILE__), "..", "../lib/falapoa/parser.rb")

# Placeholder Route when there is no protocol
get "/falapoa" do
	invalid_protocol(nil)
end

# Placeholder Route when there is no protocol
get "/falapoa/" do
	invalid_protocol(nil)
end

# PROTOCOLOS DE TESTE: "091514-12-00", "092458-12-22", "069662-12-48", "1140431137"
get "/falapoa/:protocol" do
	return invalid_protocol(params[:protocol]) if params[:protocol].length != 10
  Consumer::get_status(params[:protocol]).to_json
end

post "/falapoa/send" do
  return invalid_cause(params[:cause], params[:user]) if params[:cause].nil? || params[:user].nil?
  Consumer::send_request(params[:cause], params[:user]).to_json
end

def invalid_protocol(prot)
	{ :status => "error", :message => "Protocolo inválido", :protocol => prot }.to_json
end

def invalid_cause(cause, user)
	{ :status => "error", :message => "Causa inválida", :cause => cause, :user => user }.to_json
end