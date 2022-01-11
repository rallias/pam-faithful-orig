const pam_faithful = require("../index");
// const linuxUser = require('linux-sys-user').promise();
const os = require('os');

// beforeAll(async () => {
//     await linuxUser.addUser({ username: "pam-faithful-user", create_home: true, shell: "/bin/bash" })
//     return linuxUser.setPassword("pam-faithful-user", "pam-faithful-password");
// });

test('Tests are started as root.', () => {
    const userInfo = os.userInfo();
    console.log(userInfo);
    expect(userInfo.uid).toEqual(0);
});

// afterAll(async () => {
//     return linuxUser.removeUser("pam-faithful-user");
// });