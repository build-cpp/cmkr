// Protobuf serialization/deserialization example
// This demonstrates using protobuf with cmkr for code generation

#include <iostream>
#include <fstream>
#include <string>
#include "addressbook.pb.h"

// Creates a sample address book with some people
void create_address_book(tutorial::AddressBook &book) {
    // Add first person
    tutorial::Person *person1 = book.add_people();
    person1->set_name("Alice Smith");
    person1->set_id(1);
    person1->set_email("alice@example.com");

    // Add phone numbers for Alice
    tutorial::Person::PhoneNumber *phone1 = person1->add_phones();
    phone1->set_number("555-1234");
    phone1->set_type(tutorial::Person::HOME);

    tutorial::Person::PhoneNumber *phone2 = person1->add_phones();
    phone2->set_number("555-5678");
    phone2->set_type(tutorial::Person::WORK);

    // Add second person
    tutorial::Person *person2 = book.add_people();
    person2->set_name("Bob Johnson");
    person2->set_id(2);
    person2->set_email("bob@example.com");

    tutorial::Person::PhoneNumber *phone3 = person2->add_phones();
    phone3->set_number("555-9999");
    phone3->set_type(tutorial::Person::MOBILE);
}

// Prints the contents of an address book
void print_address_book(const tutorial::AddressBook &book) {
    std::cout << "Address Book (" << book.people_size() << " people):\n";
    std::cout << "==========================================\n\n";

    for (int i = 0; i < book.people_size(); ++i) {
        const tutorial::Person &person = book.people(i);

        std::cout << "Person ID: " << person.id() << "\n";
        std::cout << "  Name: " << person.name() << "\n";
        std::cout << "  Email: " << person.email() << "\n";

        for (int j = 0; j < person.phones_size(); ++j) {
            const tutorial::Person::PhoneNumber &phone = person.phones(j);
            std::cout << "  Phone #" << (j + 1) << ": " << phone.number() << " (";

            switch (phone.type()) {
            case tutorial::Person::MOBILE:
                std::cout << "mobile";
                break;
            case tutorial::Person::HOME:
                std::cout << "home";
                break;
            case tutorial::Person::WORK:
                std::cout << "work";
                break;
            default:
                std::cout << "unknown";
            }
            std::cout << ")\n";
        }
        std::cout << "\n";
    }
}

int main() {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::cout << "Protobuf cmkr Example\n";
    std::cout << "=====================\n\n";

    // Create an address book
    tutorial::AddressBook original_book;
    create_address_book(original_book);

    std::cout << "Original Address Book:\n";
    print_address_book(original_book);

    // Serialize to string
    std::string serialized;
    if (!original_book.SerializeToString(&serialized)) {
        std::cerr << "Error: Failed to serialize address book\n";
        return 1;
    }
    std::cout << "Serialized size: " << serialized.size() << " bytes\n\n";

    // Deserialize into a new object
    tutorial::AddressBook deserialized_book;
    if (!deserialized_book.ParseFromString(serialized)) {
        std::cerr << "Error: Failed to deserialize address book\n";
        return 1;
    }

    std::cout << "Deserialized Address Book:\n";
    print_address_book(deserialized_book);

    // Write to file
    const char *filename = "addressbook.bin";
    {
        std::ofstream output(filename, std::ios::binary);
        if (!original_book.SerializeToOstream(&output)) {
            std::cerr << "Error: Failed to write to file\n";
            return 1;
        }
    }
    std::cout << "Written to file: " << filename << "\n";

    // Read back from file
    tutorial::AddressBook file_book;
    {
        std::ifstream input(filename, std::ios::binary);
        if (!file_book.ParseFromIstream(&input)) {
            std::cerr << "Error: Failed to read from file\n";
            return 1;
        }
    }
    std::cout << "Read back from file successfully!\n\n";

    // Verify all three versions are identical
    bool all_match = true;
    if (original_book.SerializeAsString() != deserialized_book.SerializeAsString()) {
        std::cerr << "Error: Original and deserialized don't match!\n";
        all_match = false;
    }
    if (original_book.SerializeAsString() != file_book.SerializeAsString()) {
        std::cerr << "Error: Original and file don't match!\n";
        all_match = false;
    }

    if (all_match) {
        std::cout << "SUCCESS: All serialization methods produced identical results!\n";
    }

    // Clean up any global objects allocated by protobuf
    google::protobuf::ShutdownProtobufLibrary();

    return all_match ? 0 : 1;
}
