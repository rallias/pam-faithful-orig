const pam_faithful = require("../index");
const linuxUser = require('linux-sys-user').promise();
const os = require('os');


jest.setTimeout(60000);

beforeAll(async () => {
    await linuxUser.addUser({ other_args: "-l", username: "pam-faithful-user", create_home: true, shell: "/bin/bash" });
    await linuxUser.setPassword("pam-faithful-user", "pam-faithful-password");
});

test('Tests are started as root.', () => {
    const userInfo = os.userInfo();
    expect(userInfo.uid).toEqual(0);
});

afterAll(async () => {
    await linuxUser.removeUser("pam-faithful-user");
});