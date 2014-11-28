require File.join(File.dirname(__FILE__), "..", "../lib/falapoa/server.rb")
require "rack/test"
require "active_record"

set :environment, :test

def app
	Sinatra::Application
end

describe "Server (Fala Porto Alegre)" do
	include Rack::Test::Methods	

	context "Request Cause Data" do

		it "should return error message with protocol on error" do
			Consumer.stubs(:get_status).returns({:status => "error", :message => "Protocolo inválido"})
			base_error_message = { :status => "error", :message => "Protocolo inválido" }
			
			["/falapoa/", "/falapoa", "/falapoa/1234", "/falapoa/0000000000"].each do |scenario|
				get scenario
				response = ActiveSupport::JSON.decode(last_response.body)
				response["status"].should == base_error_message[:status]
				response["message"].should == base_error_message[:message]
			end
		end

		it "should Call Consumer#get_status" do
			Consumer.expects(:get_status).with("0915141200")
			get "/falapoa/0915141200"
		end

	end

	context "Send Cause" do

		before(:each) do
			@user = { "name" => "Josias Schneider", "email" => "jschneider@ionatec.com.br" }

			@cause = { "district" => "Menino Deus",
				"abstract" => "Tem que acabar com essa costrução que atrapalha o trânsito da região! (Teste de Envio de Solicitação",
				"local" => "Avenida Padre Cacique, 891, Menino Deus, 90810-240, Porto Alegre, Brasil",
				"url" => "www.portoalegre.cc/causes/1" }

			@consumer_return = {:status => "success", :data => "0924581222"}
		end

		context "success" do

			it "should call Consumer" do
				Consumer.expects(:send_request).with(@cause, @user).returns(@consumer_return)
				post "/falapoa/send", { :user => @user, :cause => @cause }
			end
			
			it "should return a protocol number" do
				Consumer.stubs(:send_request).returns(@consumer_return)
				post "/falapoa/send", { :user => @user, :cause => @cause }
				last_response.body.should == { :status => "success", :data => "0924581222" }.to_json
			end
		end

		# context "error" do
		# 	it "should not accept request without cause" do
		# 		post "/falapoa/send", { :user => @user }
		# 	end

		# 	it "should not accept request with invalid cause" do
		# 		post "/falapoa/send", { :cause => {:local => "Avenida Padre Cacique, 891, Menino Deus, 90810-240, Porto Alegre, Brasil"}, :user => @user }
		# 		post "/falapoa/send", { :cause => {:district => "Menino Deus"}, :user => @user }
		# 		post "/falapoa/send", { :cause => {:abstract => "Tem que acabar com essa costrução que atrapalha o trânsito da região! (Teste de Envio de Solicitação"}, :user => @user }
		# 		post "/falapoa/send", { :cause => {:url => "www.portoalegre.cc/causes/1"}, :user => @user }
		# 	end

		# 	it "should not accept request with invalid user" do
		# 		post "/falapoa/send", { :user => {:email => "jschneider@ionatec.com.br"} }
		# 		post "/falapoa/send", { :user => {:name => "Josias Schneider"} }
		# 	end
		# end
	end
end

