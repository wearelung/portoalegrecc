class VolunteerMailer < BaseMailer
    def send_volunteer_form(form_data)
    defaults
    subject "Seja um voluntário - Envio através do site"
    recipients ["contato@portoalegre.cc"]
    body :data => form_data
  end
  
end